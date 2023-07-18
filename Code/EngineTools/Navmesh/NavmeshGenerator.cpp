#if EE_ENABLE_NAVPOWER
#include "NavmeshGenerator.h"
#include "EngineTools/Physics/ResourceDescriptors/ResourceDescriptor_PhysicsCollisionMesh.h"
#include "EngineTools/RawAssets/RawAssetReader.h"
#include "EngineTools/RawAssets/RawMesh.h"
#include "EngineTools/Core/ToolsContext.h"
#include "Engine/Navmesh/NavPower.h"
#include "Engine/Navmesh/NavmeshData.h"
#include "Engine/Navmesh/Components/Component_Navmesh.h"
#include "Engine/Physics/Components/Component_PhysicsCollisionMesh.h"
#include "Engine/Entity/EntitySerialization.h"
#include "Engine/UpdateContext.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Base/Resource/ResourceHeader.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Serialization/BinarySerialization.h"
#include <bfxSystem.h>

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    NavmeshGenerator::NavmeshGenerator( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& rawResourceDirectoryPath, FileSystem::Path const& outputPath, EntityModel::SerializedEntityCollection const& entityCollection, NavmeshBuildSettings const& buildSettings )
        : m_rawResourceDirectoryPath( rawResourceDirectoryPath )
        , m_outputPath( outputPath )
        , m_typeRegistry( typeRegistry )
        , m_entityCollection( entityCollection )
        , m_buildSettings( buildSettings )
        , m_asyncTask( [this] ( TaskSetPartition range, uint32_t threadnum ) { GenerateSync(); } )
    {
        EE_ASSERT( rawResourceDirectoryPath.IsValid() );
        EE_ASSERT( m_outputPath.IsValid() );
    }

    NavmeshGenerator::~NavmeshGenerator()
    {
        EE_ASSERT( m_pNavpowerInstance == nullptr );
        EE_ASSERT( m_state != State::Generating );
        EE_ASSERT( !m_isGeneratingAsync );
    }

    //-------------------------------------------------------------------------

    void NavmeshGenerator::GenerateAsync( TaskSystem& taskSystem )
    {
        m_isGeneratingAsync = true;
        taskSystem.ScheduleTask( &m_asyncTask );
    }

    //-------------------------------------------------------------------------

    void NavmeshGenerator::Generate()
    {
        EE_ASSERT( m_state != State::Generating && m_pNavpowerInstance == nullptr );

        m_collisionPrimitives.clear();
        m_buildFaces.clear();
        m_numCollisionPrimitivesToProcess = 0;
        m_progressMessage[0] = 0;
        m_progress = 0.0f;
        m_state = State::Generating;

        //-------------------------------------------------------------------------

        if ( CollectCollisionPrimitives() )
        {
            if ( CollectTriangles() )
            {
                NavmeshData navmeshData;
                if ( BuildNavmesh( navmeshData ) )
                {
                    if ( SaveNavmesh( navmeshData ) )
                    {
                        m_state = State::CompletedSuccess;
                        m_isGeneratingAsync = false;
                        return;
                    }
                }
            }
        }

        //-------------------------------------------------------------------------

        m_state = State::CompletedFailure;
        m_isGeneratingAsync = false;
    }

    //-------------------------------------------------------------------------

    bool NavmeshGenerator::CollectCollisionPrimitives()
    {
        Printf( m_progressMessage, 256, "Step 1/4: Collecting Primitives" );
        m_progress = 0.0f;

        TVector<Entity*> createdEntities = EntityModel::Serializer::CreateEntities( nullptr, m_typeRegistry, m_entityCollection );

        // Update all spatial transforms
        //-------------------------------------------------------------------------
        // We need to do this since we dont update world transforms when loading a collection

        for ( auto pEntity : createdEntities )
        {
            if ( pEntity->IsSpatialEntity() )
            {
                pEntity->SetWorldTransform( pEntity->GetWorldTransform() );
            }
        }

        size_t numComponentsToProgress = 0;

        // Collect all collision geometry
        //-------------------------------------------------------------------------

        auto foundPhysicsComponents = m_entityCollection.GetComponentsOfType( m_typeRegistry, Physics::CollisionMeshComponent::GetStaticTypeID() );
        numComponentsToProgress += foundPhysicsComponents.size();

        float cnt = 0;
        for ( auto const& searchResult : foundPhysicsComponents )
        {
            int32_t const entityIdx = m_entityCollection.FindEntityIndex( searchResult.m_pEntity->m_name );
            EE_ASSERT( entityIdx != InvalidIndex );

            int32_t const componentIdx = m_entityCollection.GetEntityDescriptors()[entityIdx].FindComponentIndex( searchResult.m_pComponent->m_name );
            EE_ASSERT( componentIdx != InvalidIndex );

            Entity const* pEntity = createdEntities[entityIdx];
            EE_ASSERT( pEntity != nullptr );

            auto pPhysicsComponent = Cast<Physics::CollisionMeshComponent>( pEntity->GetComponents()[componentIdx] );
            EE_ASSERT( pPhysicsComponent != nullptr );

            ResourceID geometryResourceID = pPhysicsComponent->GetCollisionResourceID();
            if ( geometryResourceID.IsValid() )
            {
                CollisionMesh const cm = { pPhysicsComponent->GetWorldTransform(), Vector( pPhysicsComponent->GetLocalScale() ) };

                if ( cm.m_localScale.IsAnyEqualToZero3() )
                {
                    EE_LOG_WARNING( "Navmesh", "Generation", "Detected 0 local scale for physics collision component ( %s ). This is not supported so collision will be skipped!", pPhysicsComponent->GetNameID().c_str() );
                }
                else
                {
                    m_collisionPrimitives[geometryResourceID.GetResourcePath()].emplace_back( cm );
                    m_numCollisionPrimitivesToProcess++;
                }
            }

            // Update progress
            cnt++;
            m_progress = cnt / numComponentsToProgress;
        }

        // Collect all inclusion/exclusion volumes too
        //-------------------------------------------------------------------------

        // TODO

        //-------------------------------------------------------------------------

        for ( auto& pEntity : createdEntities )
        {
            EE::Delete( pEntity );
        }

        return true;
    }

    bool NavmeshGenerator::CollectTriangles()
    {
        auto LogError = [] ( char const* pFormat, ... )
        {
            va_list args;
            va_start( args, pFormat );
            Log::AddEntryVarArgs( Log::Severity::Error, "Navmesh", "Generation", __FILE__, __LINE__, pFormat, args );
            va_end( args );
            return false;
        };

        //-------------------------------------------------------------------------

        Printf( m_progressMessage, 256, "Step 2/4: Collecting Triangles" );
        m_progress = 0.0f;

        float cnt = 0;
        for ( auto const& primitiveDesc : m_collisionPrimitives )
        {
            // Load descriptor
            //-------------------------------------------------------------------------

            FileSystem::Path meshDescriptorFilePath;
            if ( primitiveDesc.first.IsValid() )
            {
                meshDescriptorFilePath = ResourcePath::ToFileSystemPath( m_rawResourceDirectoryPath, primitiveDesc.first );
            }
            else
            {
                return LogError( "Invalid source data path (%s) for physics mesh descriptor", primitiveDesc.first.c_str() );
            }

            Physics::PhysicsCollisionMeshResourceDescriptor resourceDescriptor;
            if ( !Resource::ResourceDescriptor::TryReadFromFile( m_typeRegistry, meshDescriptorFilePath, resourceDescriptor ) )
            {
                return LogError( "Failed to read physics mesh resource descriptor from file: %s", meshDescriptorFilePath.c_str() );
            }

            // Load mesh
            //-------------------------------------------------------------------------

            FileSystem::Path meshFilePath;
            if ( primitiveDesc.first.IsValid() )
            {
                meshFilePath = ResourcePath::ToFileSystemPath( m_rawResourceDirectoryPath, resourceDescriptor.m_sourcePath );
            }
            else
            {
                return LogError( "Invalid source data path (%) in physics collision descriptor: %s", resourceDescriptor.m_sourcePath.c_str(), meshDescriptorFilePath.c_str() );
            }

            RawAssets::ReaderContext readerCtx = 
            { 
                [this] ( char const* pString ) { EE_LOG_WARNING( "Navmesh", "Generation", pString ); },
                [this] ( char const* pString ) { EE_LOG_ERROR( "Navmesh", "Generation", pString ); }
            };

            TVector<String> meshesToInclude;
            if ( !resourceDescriptor.m_sourceItemName.empty() )
            {
                meshesToInclude.emplace_back( resourceDescriptor.m_sourceItemName );
            }

            TUniquePtr<RawAssets::RawMesh> pRawMesh = RawAssets::ReadStaticMesh( readerCtx, meshFilePath, meshesToInclude );
            if ( pRawMesh == nullptr )
            {
                return LogError( "Failed to read mesh from source file: %s" );
            }

            EE_ASSERT( pRawMesh->IsValid() );

            // Add triangles
            //-------------------------------------------------------------------------

            for ( CollisionMesh const& cm : primitiveDesc.second )
            {
                Float3 const finalScale = ( cm.m_localScale * cm.m_worldTransform.GetScale() ).ToFloat3();
 
                int32_t numNegativelyScaledAxes = ( finalScale.m_x < 0 ) ? 1 : 0;
                numNegativelyScaledAxes += ( finalScale.m_y < 0 ) ? 1 : 0;
                numNegativelyScaledAxes += ( finalScale.m_z < 0 ) ? 1 : 0;

                bool const flipWindingDueToScale = Math::IsOdd( numNegativelyScaledAxes );

                //-------------------------------------------------------------------------

                Matrix meshTransform = cm.m_worldTransform.ToMatrixNoScale();
                meshTransform.SetScale( finalScale );

                //-------------------------------------------------------------------------

                for ( auto const& geometrySection : pRawMesh->GetGeometrySections() )
                {
                    // NavPower expects counterclockwise winding
                    bool flipWinding = geometrySection.m_clockwiseWinding ? true : false;
                    if ( flipWindingDueToScale )
                    {
                        flipWinding = !flipWinding;
                    }

                    //-------------------------------------------------------------------------

                    int32_t const numTriangles = geometrySection.GetNumTriangles();
                    int32_t const numIndices = (int32_t) geometrySection.m_indices.size();
                    for ( auto t = 0; t < numTriangles; t++ )
                    {
                        int32_t const i = t * 3;
                        EE_ASSERT( i <= numIndices - 3 );

                        // NavPower expects counterclockwise winding
                        int32_t const index0 = geometrySection.m_indices[flipWinding ? i + 2 : i];
                        int32_t const index1 = geometrySection.m_indices[i + 1];
                        int32_t const index2 = geometrySection.m_indices[flipWinding ? i : i + 2];

                        // Add triangle
                        auto& buildFace = m_buildFaces.emplace_back( bfx::BuildFace() );
                        buildFace.m_type = bfx::WALKABLE_FACE;

                        buildFace.m_verts[0] = ToBfx( meshTransform.TransformPoint( geometrySection.m_vertices[index0].m_position ) );
                        buildFace.m_verts[1] = ToBfx( meshTransform.TransformPoint( geometrySection.m_vertices[index1].m_position ) );
                        buildFace.m_verts[2] = ToBfx( meshTransform.TransformPoint( geometrySection.m_vertices[index2].m_position ) );
                    }
                }

                // Update progress
                cnt++;
                m_progress = cnt / m_numCollisionPrimitivesToProcess;
            }
        }

        return true;
    }

    bool NavmeshGenerator::BuildNavmesh( NavmeshData& navmeshData )
    {
        Printf( m_progressMessage, 256, "Step 3/4: Building Navmesh" );
        m_progress = 0.0f;

        if ( m_buildFaces.empty() )
        {
            return true;
        }

        //-------------------------------------------------------------------------

        bfx::CustomAllocator* pAllocator = bfx::CreateDLMallocAllocator();
        m_pNavpowerInstance = bfx::SystemCreate( bfx::SystemParams( 2.0f, bfx::Z_UP ), pAllocator );

        bfx::SetCurrentInstance( m_pNavpowerInstance );
        bfx::RegisterBuilderSystem();
        bfx::SystemStart( m_pNavpowerInstance );

        //-------------------------------------------------------------------------

        TVector<bfx::BuildParams> layerBuildParams;

        // Default Layer
        auto& defaultLayerBuildParams = layerBuildParams.emplace_back( bfx::BuildParams() );
        defaultLayerBuildParams.m_voxSize = m_buildSettings.m_defaultLayerBuildSettings.m_voxSize;
        defaultLayerBuildParams.m_height = m_buildSettings.m_defaultLayerBuildSettings.m_height;
        defaultLayerBuildParams.m_radius = m_buildSettings.m_defaultLayerBuildSettings.m_radius;
        defaultLayerBuildParams.m_step = m_buildSettings.m_defaultLayerBuildSettings.m_step;
        defaultLayerBuildParams.m_additionalInwardsSmoothingDist = m_buildSettings.m_defaultLayerBuildSettings.m_additionalInwardsSmoothingDist;
        defaultLayerBuildParams.m_optimizeForAxisAligned = m_buildSettings.m_defaultLayerBuildSettings.m_optimizeForAxisAligned;
        defaultLayerBuildParams.m_dropOffRadius = m_buildSettings.m_defaultLayerBuildSettings.m_dropOffRadius;
        defaultLayerBuildParams.m_maxWalkableSlope = m_buildSettings.m_defaultLayerBuildSettings.m_maxWalkableSlope;
        defaultLayerBuildParams.m_leaveSmallIslandsTouchingPortals = m_buildSettings.m_defaultLayerBuildSettings.m_leaveSmallIslandsTouchingPortals;
        defaultLayerBuildParams.m_minIslandSurfaceArea = m_buildSettings.m_defaultLayerBuildSettings.m_minIslandSurfaceArea;
        defaultLayerBuildParams.m_useEnhancedTerrainTracking = m_buildSettings.m_defaultLayerBuildSettings.m_useEnhancedTerrainTracking;
        defaultLayerBuildParams.m_tessellateForPathingAccuracy = m_buildSettings.m_defaultLayerBuildSettings.m_tessellateForPathingAccuracy;

        // Additional Layers
        for ( auto const& layerSettings : m_buildSettings.m_additionalLayerBuildSettings )
        {
            auto& buildParams = layerBuildParams.emplace_back( bfx::BuildParams() );
            buildParams.m_voxSize = layerSettings.m_voxSize;
            buildParams.m_height = layerSettings.m_height;
            buildParams.m_radius = layerSettings.m_radius;
            buildParams.m_step = layerSettings.m_step;
            buildParams.m_additionalInwardsSmoothingDist = layerSettings.m_additionalInwardsSmoothingDist;
            buildParams.m_optimizeForAxisAligned = layerSettings.m_optimizeForAxisAligned;
            buildParams.m_dropOffRadius = layerSettings.m_dropOffRadius;
            buildParams.m_maxWalkableSlope = layerSettings.m_maxWalkableSlope;
            buildParams.m_leaveSmallIslandsTouchingPortals = layerSettings.m_leaveSmallIslandsTouchingPortals;
            buildParams.m_minIslandSurfaceArea = layerSettings.m_minIslandSurfaceArea;
            buildParams.m_useEnhancedTerrainTracking = layerSettings.m_useEnhancedTerrainTracking;
            buildParams.m_tessellateForPathingAccuracy = layerSettings.m_tessellateForPathingAccuracy;
        }

        //-------------------------------------------------------------------------

        bfx::SurfaceNavigationInput surfaceInput;
        surfaceInput.m_globalParams.m_maxNumCores = 16;

        surfaceInput.m_pFaces = m_buildFaces.data();
        surfaceInput.m_numFaces = (uint32_t) m_buildFaces.size();
        surfaceInput.m_pParams = layerBuildParams.data();
        surfaceInput.m_numParams = (uint32_t) layerBuildParams.size();

        surfaceInput.m_globalParams.m_enableMulticoreBuild = true;
        surfaceInput.m_globalParams.m_maxNumCores = Threading::GetProcessorInfo().m_numPhysicalCores / 2;
        surfaceInput.m_globalParams.m_pMonitor = this;

        //-------------------------------------------------------------------------

        if ( m_buildSettings.m_enableBuildLogging )
        {
            FileSystem::Path buildLogPath = m_outputPath;
            buildLogPath.ReplaceExtension( "bfx_log" );
            bfx::EnableBuildLog( buildLogPath.c_str(), true );
        }

        bfx::NavGraphImage* pGraphImage = bfx::CreateNavGraphImage( surfaceInput, bfx::PlatformParams() );

        // Copy graph image data into the resource
        navmeshData.m_graphImage.resize( pGraphImage->GetNumBytes() );
        memcpy( navmeshData.m_graphImage.data(), pGraphImage->GetPtr(), pGraphImage->GetNumBytes() );

        bfx::DestroyNavGraphImage( pGraphImage );

        //-------------------------------------------------------------------------

        bfx::SystemStop( m_pNavpowerInstance );
        bfx::SystemDestroy( m_pNavpowerInstance );
        bfx::DestroyAllocator( pAllocator );
        m_pNavpowerInstance = nullptr;

        return true;
    }

    bool NavmeshGenerator::SaveNavmesh( NavmeshData& navmeshData )
    {
        Printf( m_progressMessage, 256, "Step 4/4: Saving Navmesh" );
        m_progress = 1.0f;

        Serialization::BinaryOutputArchive archive;
        archive << Resource::ResourceHeader( s_version, Navmesh::NavmeshData::GetStaticResourceTypeID(), 0 ) << navmeshData;

        if ( archive.WriteToFile( m_outputPath ) )
        {
            return true;
        }

        return false;
    }
}
#endif