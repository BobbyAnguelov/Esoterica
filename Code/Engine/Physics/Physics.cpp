#include "Physics.h"
#include "omnipvd/PxOmniPvd.h"
#include "System/Types/Arrays.h"

//-------------------------------------------------------------------------

using namespace physx;

//-------------------------------------------------------------------------

namespace EE::Physics
{
    Float3 const Constants::s_gravity = Float3( 0, 0, -9.81f );
}

//-------------------------------------------------------------------------
// PhysX Global State
//-------------------------------------------------------------------------

namespace EE::Physics::PX
{
    EE::Quaternion const Conversion::s_capsuleConversionToPx = Quaternion( EulerAngles( 0, 90, 0 ) );
    EE::Quaternion const Conversion::s_capsuleConversionFromPx = Quaternion( EulerAngles( 0, -90, 0 ) );

    //-------------------------------------------------------------------------

    struct GlobalState
    {
        void Initialize()
        {
            EE_ASSERT( m_pFoundation == nullptr && m_pPhysics == nullptr );

            PxTolerancesScale tolerancesScale;
            tolerancesScale.length = Constants::s_lengthScale;
            tolerancesScale.speed = Constants::s_speedScale;

            //-------------------------------------------------------------------------

            m_pAllocatorCallback = EE::New<Allocator>();
            m_pErrorCallback = EE::New<UserErrorCallback>();

            m_pFoundation = PxCreateFoundation( PX_PHYSICS_VERSION, *m_pAllocatorCallback, *m_pErrorCallback );
            EE_ASSERT( m_pFoundation != nullptr );

            #if EE_DEVELOPMENT_TOOLS
            m_pPVD = PxCreateOmniPvd( *m_pFoundation );
            m_pPhysics = PxCreatePhysics( PX_PHYSICS_VERSION, *m_pFoundation, tolerancesScale, true, nullptr, m_pPVD );
            #else
            m_pPhysics = PxCreatePhysics( PX_PHYSICS_VERSION, *m_pFoundation, tolerancesScale, true );
            #endif

            m_pCooking = PxCreateCooking( PX_PHYSICS_VERSION, *m_pFoundation, PxCookingParams( tolerancesScale ) );
        }

        void Shutdown()
        {
            EE_ASSERT( m_pFoundation != nullptr && m_pPhysics != nullptr );

            m_pCooking->release();
            m_pPhysics->release();
            m_pFoundation->release();

            EE::Delete( m_pErrorCallback );
            EE::Delete( m_pAllocatorCallback );
        }

    public:

        physx::PxFoundation*                            m_pFoundation = nullptr;
        physx::PxPhysics*                               m_pPhysics = nullptr;
        physx::PxCooking*                               m_pCooking = nullptr;
        physx::PxAllocatorCallback*                     m_pAllocatorCallback = nullptr;
        physx::PxErrorCallback*                         m_pErrorCallback = nullptr;
        physx::PxSimulationEventCallback*               m_pEventCallbackHandler = nullptr;

        #if EE_DEVELOPMENT_TOOLS
        physx::PxOmniPvd*                               m_pPVD;
        #endif
    };
}

//-------------------------------------------------------------------------
// Shared Resources
//-------------------------------------------------------------------------

namespace EE::Physics::PX
{
    // Creates a unit cylinder mesh with the specified number of side
    // Half-height is along the Z axis, Radius is in the X/Y plane
    static PxConvexMesh* CreateSharedCylinderMesh( PxPhysics* pPhysics, PxCooking* pCooking, uint32_t numSides )
    {
        EE_ASSERT( pPhysics != nullptr && pCooking != nullptr );

        //-------------------------------------------------------------------------

        float const anglePerSide( Math::TwoPi / numSides );
        PxVec3 const rotationAxis( 0, 0, 1 );
        PxVec3 const vertVector( 0, 0, -0.5f );
        PxVec3 const cylinderTopOffset( 0, 0, 0.5f );
        PxVec3 const cylinderBottomOffset( 0, 0, -0.5f );

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

    //-------------------------------------------------------------------------

    physx::PxConvexMesh* Shapes::s_pUnitCylinderMesh = nullptr;
}

//-------------------------------------------------------------------------
// Physics System
//-------------------------------------------------------------------------

namespace EE::Physics::Core
{
    static PX::GlobalState* g_pGlobalState = nullptr;

    //-------------------------------------------------------------------------

    void Initialize()
    {
        // Global State
        EE_ASSERT( g_pGlobalState == nullptr );
        g_pGlobalState = EE::New<PX::GlobalState>();
        g_pGlobalState->Initialize();

        // Create shared resources
        EE_ASSERT( PX::Shapes::s_pUnitCylinderMesh == nullptr );
        PX::Shapes::s_pUnitCylinderMesh = PX::CreateSharedCylinderMesh( g_pGlobalState->m_pPhysics, g_pGlobalState->m_pCooking, 30 );
    }

    void Shutdown()
    {
        // Destroy shared resources
        PX::Shapes::s_pUnitCylinderMesh->release();
        PX::Shapes::s_pUnitCylinderMesh = nullptr;

        // Global State
        EE_ASSERT( g_pGlobalState != nullptr );
        g_pGlobalState->Shutdown();
        EE::Delete( g_pGlobalState );
    }

    physx::PxPhysics* GetPxPhysics()
    {
        return g_pGlobalState->m_pPhysics;
    }
}