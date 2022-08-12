#include "PhysicsSystem.h"
#include "PhysicsScene.h"
#include "PhysicsSimulationFilter.h"
#include "System/Profiling.h"
#include "PhysX.h"

//-------------------------------------------------------------------------

using namespace physx;

//-------------------------------------------------------------------------
// Shared Mesh Constructors
//-------------------------------------------------------------------------

namespace EE::Physics
{
    // Creates a unit cylinder mesh with the specified number of side
    // Half-height is along the X axis, Radius is in the Y/Z plane
    static PxConvexMesh* CreateSharedCylinderMesh( PxPhysics* pPhysics, PxCooking* pCooking, uint32_t numSides )
    {
        EE_ASSERT( pPhysics != nullptr && pCooking != nullptr );

        //-------------------------------------------------------------------------

        float const anglePerSide( Math::TwoPi / numSides );
        PxVec3 const rotationAxis( 1, 0, 0 );
        PxVec3 const vertVector( 0, 0, -0.5f );
        PxVec3 const cylinderTopOffset( -0.5f, 0, 0 );
        PxVec3 const cylinderBottomOffset( 0.5f, 0, 0 );

        //-------------------------------------------------------------------------

        TVector<PxVec3> convexVerts;
        convexVerts.resize( ( numSides + 1 ) * 2 );

        convexVerts[0] = vertVector + cylinderTopOffset;
        convexVerts[numSides] = vertVector + cylinderBottomOffset;

        float currentAngle = anglePerSide;
        for ( uint32_t i = 1; i < numSides; i++ )
        {
            PxQuat const rotation( currentAngle, rotationAxis );
            PxVec3 const rotatedVertVector = rotation.rotate( vertVector );

            convexVerts[i] = rotatedVertVector + cylinderTopOffset;
            convexVerts[numSides + i] = rotatedVertVector + cylinderBottomOffset;

            currentAngle += anglePerSide;
        }

        //-------------------------------------------------------------------------

        PxConvexMeshDesc convexDesc;
        convexDesc.points.count = (uint32_t) convexVerts.size();
        convexDesc.points.stride = sizeof( PxVec3 );
        convexDesc.points.data = convexVerts.data();
        convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

        PxDefaultMemoryOutputStream buffer;
        PxConvexMeshCookingResult::Enum result;
        if ( !pCooking->cookConvexMesh( convexDesc, buffer, &result ) )
        {
            EE_UNREACHABLE_CODE();
        }

        PxDefaultMemoryInputData input( buffer.getData(), buffer.getSize() );
        PxConvexMesh* pCylinderMesh = pPhysics->createConvexMesh( input );
        EE_ASSERT( pCylinderMesh != nullptr );
        return pCylinderMesh;
    }
}

//-------------------------------------------------------------------------
// System
//-------------------------------------------------------------------------

namespace EE::Physics
{
    void PhysicsSystem::Initialize()
    {
        EE_ASSERT( m_pFoundation == nullptr && m_pPhysics == nullptr && m_pDispatcher == nullptr );

        PxTolerancesScale tolerancesScale;
        tolerancesScale.length = Constants::s_lengthScale;
        tolerancesScale.speed = Constants::s_speedScale;

        //-------------------------------------------------------------------------

        m_pAllocatorCallback = EE::New<PhysXAllocator>();
        m_pErrorCallback = EE::New<PhysXUserErrorCallback>();

        m_pFoundation = PxCreateFoundation( PX_PHYSICS_VERSION, *m_pAllocatorCallback, *m_pErrorCallback );
        EE_ASSERT( m_pFoundation != nullptr );
        m_pDispatcher = EE::New<PhysXTaskDispatcher>();
        m_pSimulationFilterCallback = EE::New<SimulationFilter>();

        #if EE_DEVELOPMENT_TOOLS
        m_pPVD = PxCreatePvd( *m_pFoundation );
        m_pPVDTransport = PxDefaultPvdSocketTransportCreate( "127.0.0.1", 5425, 10 );
        m_pPhysics = PxCreatePhysics( PX_PHYSICS_VERSION, *m_pFoundation, tolerancesScale, true, m_pPVD );
        #else
        m_pPhysics = PxCreatePhysics( PX_PHYSICS_VERSION, *m_pFoundation, tolerancesScale, true, nullptr );
        #endif

        m_pCooking = PxCreateCooking( PX_PHYSICS_VERSION, *m_pFoundation, PxCookingParams( tolerancesScale ) );

        // Create shared resources
        //-------------------------------------------------------------------------

        CreateSharedMeshes();
    }

    void PhysicsSystem::Shutdown()
    {
        EE_ASSERT( m_pFoundation != nullptr && m_pPhysics != nullptr && m_pDispatcher != nullptr );

        #if EE_DEVELOPMENT_TOOLS
        if ( m_pPVD->isConnected() )
        {
            m_pPVD->disconnect();
        }
        #endif

        //-------------------------------------------------------------------------

        DestroySharedMeshes();

        //-------------------------------------------------------------------------

        m_pCooking->release();
        m_pPhysics->release();

        #if EE_DEVELOPMENT_TOOLS
        m_pPVDTransport->release();
        m_pPVD->release();
        #endif

        EE::Delete( m_pSimulationFilterCallback );
        EE::Delete( m_pDispatcher );
        m_pFoundation->release();

        EE::Delete( m_pErrorCallback );
        EE::Delete( m_pAllocatorCallback );
    }

    //-------------------------------------------------------------------------

    Scene* PhysicsSystem::CreateScene()
    {
        PxTolerancesScale tolerancesScale;
        tolerancesScale.length = Constants::s_lengthScale;
        tolerancesScale.speed = Constants::s_speedScale;

        PxSceneDesc sceneDesc( tolerancesScale );
        sceneDesc.gravity = ToPx( Constants::s_gravity );
        sceneDesc.cpuDispatcher = m_pDispatcher;
        sceneDesc.filterShader = SimulationFilter::Shader;
        sceneDesc.filterCallback = m_pSimulationFilterCallback;
        sceneDesc.flags = PxSceneFlag::eENABLE_CCD | PxSceneFlag::eREQUIRE_RW_LOCK;
        auto pPxScene = m_pPhysics->createScene( sceneDesc );

        #if EE_DEVELOPMENT_TOOLS
        auto pPvdClient = pPxScene->getScenePvdClient();
        pPvdClient->setScenePvdFlag( PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true );
        pPvdClient->setScenePvdFlag( PxPvdSceneFlag::eTRANSMIT_CONTACTS, true );
        pPvdClient->setScenePvdFlag( PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true );
        #endif

        return EE::New<Scene>( pPxScene );
    }

    //-------------------------------------------------------------------------

    void PhysicsSystem::CreateSharedMeshes()
    {
        EE_ASSERT( m_pCooking != nullptr );
        EE_ASSERT( SharedMeshes::s_pUnitCylinderMesh == nullptr );
        SharedMeshes::s_pUnitCylinderMesh = CreateSharedCylinderMesh( m_pPhysics, m_pCooking, 30 );
    }

    void PhysicsSystem::DestroySharedMeshes()
    {
        SharedMeshes::s_pUnitCylinderMesh->release();
        SharedMeshes::s_pUnitCylinderMesh = nullptr;
    }

    //-------------------------------------------------------------------------

    void PhysicsSystem::Update( UpdateContext& ctx )
    {
        #if EE_DEVELOPMENT_TOOLS
        if ( m_recordingTimeLeft >= 0 )
        {
            if ( m_pPVD->isConnected() )
            {
                m_recordingTimeLeft -= ctx.GetDeltaTime();
                if ( m_recordingTimeLeft < 0 )
                {
                    m_pPVD->disconnect();
                    m_recordingTimeLeft = -1.0f;
                }
            }
            else
            {
                m_recordingTimeLeft = -1.0f;
            }
        }
        #endif
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    bool PhysicsSystem::IsConnectedToPVD()
    {
        EE_ASSERT( m_pPVD != nullptr );
        return m_pPVD->isConnected();
    }

    void PhysicsSystem::ConnectToPVD( Seconds timeToRecord )
    {
        EE_ASSERT( m_pPVD != nullptr );
        if ( !m_pPVD->isConnected() )
        {
            m_pPVD->connect( *m_pPVDTransport, PxPvdInstrumentationFlag::eALL );
            m_recordingTimeLeft = timeToRecord;
        }
    }

    void PhysicsSystem::DisconnectFromPVD()
    {
        EE_ASSERT( m_pPVD != nullptr );
        if ( m_pPVD->isConnected() )
        {
            m_pPVD->disconnect();
        }
    }
    #endif
}

//-------------------------------------------------------------------------
// Physical Materials
//-------------------------------------------------------------------------

namespace EE::Physics
{
    void PhysicsSystem::FillMaterialDatabase( TVector<PhysicsMaterialSettings> const& materials )
    {
        // Create default physics material
        StringID const defaultMaterialID( PhysicsMaterial::DefaultID );
        m_pDefaultMaterial = m_pPhysics->createMaterial( PhysicsMaterial::DefaultStaticFriction, PhysicsMaterial::DefaultDynamicFriction, PhysicsMaterial::DefaultRestitution );
        m_materials.insert( TPair<StringID, PhysicsMaterial>( defaultMaterialID, PhysicsMaterial( defaultMaterialID, m_pDefaultMaterial ) ) );

        // Create physX materials
        physx::PxMaterial* pxMaterial = nullptr;
        for ( auto const& materialSettings : materials )
        {
            pxMaterial = m_pPhysics->createMaterial( materialSettings.m_staticFriction, materialSettings.m_dynamicFriction, materialSettings.m_restitution );
            pxMaterial->setFrictionCombineMode( (PxCombineMode::Enum) materialSettings.m_frictionCombineMode );
            pxMaterial->setRestitutionCombineMode( (PxCombineMode::Enum) materialSettings.m_restitutionCombineMode );

            m_materials.insert( TPair<StringID, PhysicsMaterial>( materialSettings.m_ID, PhysicsMaterial( materialSettings.m_ID, pxMaterial ) ) );
        }
    }

    void PhysicsSystem::ClearMaterialDatabase()
    {
        for ( auto& materialPair : m_materials )
        {
            materialPair.second.m_pMaterial->release();
            materialPair.second.m_pMaterial = nullptr;
        }

        m_materials.clear();
    }

    PxMaterial* PhysicsSystem::GetMaterial( StringID materialID ) const
    {
        EE_ASSERT( materialID.IsValid() );

        auto foundMaterialPairIter = m_materials.find( materialID );
        if ( foundMaterialPairIter != m_materials.end() )
        {
            return foundMaterialPairIter->second.m_pMaterial;
        }

        EE_LOG_WARNING( "Physics", "Physics System", "Failed to find physic material with ID: %s", materialID.c_str() );
        return nullptr;
    }
}