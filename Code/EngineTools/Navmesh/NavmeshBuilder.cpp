#if EE_ENABLE_NAVPOWER
#include "NavmeshBuilder.h"
#include "NavmeshBuildData.h"
#include "Engine/Navmesh/NavPower.h"
#include <bfxSystem.h>

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    bool NavmeshBuilder::Build( NavmeshBuildData const& buildData, NavmeshData& outData )
    {
        ClearLog();

        m_buildFaces.clear();
        m_progress = 0.0f;
        outData.m_graphImage.clear();

        if ( !buildData.IsReadyToBuild() )
        {
            return false;
        }

        if ( !CollectTriangles( buildData ) )
        {
            return false;
        }

        if ( !BuildNavmesh( buildData, outData ) )
        {
            return false;
        }

        return true;
    }

    bool NavmeshBuilder::CollectTriangles( NavmeshBuildData const& buildData )
    {
        m_progress = 0.0f;

        float const numInstances = (float) buildData.m_collisionMeshInstances.size();
        float cnt = 0;
        for ( auto const& meshInstanceList : buildData.m_collisionMeshInstances )
        {
            auto const foundMeshIter = buildData.m_importedCollisionMeshes.find( meshInstanceList.first );
            if ( foundMeshIter == buildData.m_importedCollisionMeshes.end() )
            {
                continue;
            }

            Import::Mesh const* pImportedMesh = foundMeshIter->second;
            EE_ASSERT( pImportedMesh != nullptr );

            // Add triangles
            //-------------------------------------------------------------------------

            for ( NavmeshBuildData::MeshInstance const& mi : meshInstanceList.second )
            {
                Float3 const finalScale = ( mi.m_nonUniformScale * mi.m_worldTransform.GetScale() ).ToFloat3();
 
                int32_t numNegativelyScaledAxes = ( finalScale.m_x < 0 ) ? 1 : 0;
                numNegativelyScaledAxes += ( finalScale.m_y < 0 ) ? 1 : 0;
                numNegativelyScaledAxes += ( finalScale.m_z < 0 ) ? 1 : 0;

                bool const flipWindingDueToScale = Math::IsOdd( numNegativelyScaledAxes );

                //-------------------------------------------------------------------------

                Matrix meshTransform = mi.m_worldTransform.ToMatrixNoScale();
                meshTransform.SetScale( finalScale );

                //-------------------------------------------------------------------------

                auto const& meshGeometries = pImportedMesh->GetGeometries();

                for ( auto const& submesh : pImportedMesh->GetSubmeshes() )
                {
                    auto const& geo = meshGeometries[submesh.m_geometryIdx];

                    // NavPower expects counterclockwise winding
                    bool flipWinding = geo.m_clockwiseWinding ? true : false;
                    if ( flipWindingDueToScale )
                    {
                        flipWinding = !flipWinding;
                    }

                    //-------------------------------------------------------------------------

                    int32_t const numTriangles = geo.GetNumTriangles();
                    int32_t const numIndices = (int32_t) geo.m_indices.size();
                    for ( auto t = 0; t < numTriangles; t++ )
                    {
                        int32_t const i = t * 3;
                        EE_ASSERT( i <= numIndices - 3 );

                        // NavPower expects counterclockwise winding
                        int32_t const index0 = geo.m_indices[flipWinding ? i + 2 : i];
                        int32_t const index1 = geo.m_indices[i + 1];
                        int32_t const index2 = geo.m_indices[flipWinding ? i : i + 2];

                        // Add triangle
                        auto& buildFace = m_buildFaces.emplace_back( bfx::BuildFace() );
                        buildFace.m_type = bfx::WALKABLE_FACE;

                        buildFace.m_verts[0] = ToBfx( meshTransform.TransformPoint( submesh.m_transform.TransformPoint( geo.m_vertices[index0].m_position ) ) );
                        buildFace.m_verts[1] = ToBfx( meshTransform.TransformPoint( submesh.m_transform.TransformPoint( geo.m_vertices[index1].m_position ) ) );
                        buildFace.m_verts[2] = ToBfx( meshTransform.TransformPoint( submesh.m_transform.TransformPoint( geo.m_vertices[index2].m_position ) ) );
                    }
                }

                // Update progress
                cnt++;
                m_progress = cnt / numInstances;
            }
        }

        return true;
    }

    bool NavmeshBuilder::BuildNavmesh( NavmeshBuildData const& buildData, NavmeshData& navmeshData )
    {
        m_progress = 0.0f;

        if ( m_buildFaces.empty() )
        {
            m_progress = 1.0f;
            return true;
        }

        //-------------------------------------------------------------------------

        bfx::CustomAllocator* pAllocator = bfx::CreateDLMallocAllocator();
        m_pNavpowerInstance = bfx::SystemCreate( bfx::SystemParams( 2.0f, bfx::Z_UP ), pAllocator );

        bfx::SetCurrentInstance( m_pNavpowerInstance );
        bfx::RegisterBuilderSystem();
        bfx::SystemStart( m_pNavpowerInstance );

        //-------------------------------------------------------------------------

        NavmeshBuildSettings const& buildSettings = buildData.m_buildSettings;
        TVector<bfx::BuildParams> layerBuildParams;

        // Default Layer
        auto& defaultLayerBuildParams = layerBuildParams.emplace_back( bfx::BuildParams() );
        defaultLayerBuildParams.m_voxSize = buildSettings.m_defaultLayerBuildSettings.m_voxSize;
        defaultLayerBuildParams.m_height = buildSettings.m_defaultLayerBuildSettings.m_height;
        defaultLayerBuildParams.m_radius = buildSettings.m_defaultLayerBuildSettings.m_radius;
        defaultLayerBuildParams.m_step = buildSettings.m_defaultLayerBuildSettings.m_step;
        defaultLayerBuildParams.m_additionalInwardsSmoothingDist = buildSettings.m_defaultLayerBuildSettings.m_additionalInwardsSmoothingDist;
        defaultLayerBuildParams.m_optimizeForAxisAligned = buildSettings.m_defaultLayerBuildSettings.m_optimizeForAxisAligned;
        defaultLayerBuildParams.m_dropOffRadius = buildSettings.m_defaultLayerBuildSettings.m_dropOffRadius;
        defaultLayerBuildParams.m_maxWalkableSlope = buildSettings.m_defaultLayerBuildSettings.m_maxWalkableSlope;
        defaultLayerBuildParams.m_leaveSmallIslandsTouchingPortals = buildSettings.m_defaultLayerBuildSettings.m_leaveSmallIslandsTouchingPortals;
        defaultLayerBuildParams.m_minIslandSurfaceArea = buildSettings.m_defaultLayerBuildSettings.m_minIslandSurfaceArea;
        defaultLayerBuildParams.m_useEnhancedTerrainTracking = buildSettings.m_defaultLayerBuildSettings.m_useEnhancedTerrainTracking;
        defaultLayerBuildParams.m_tessellateForPathingAccuracy = buildSettings.m_defaultLayerBuildSettings.m_tessellateForPathingAccuracy;

        // Additional Layers
        for ( auto const& layerSettings : buildSettings.m_additionalLayerBuildSettings )
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
        surfaceInput.m_globalParams.m_maxNumCores = Math::Max( 1, Threading::GetProcessorInfo().m_numPhysicalCores - 2 );
        surfaceInput.m_globalParams.m_pMonitor = this;

        //-------------------------------------------------------------------------

        if ( buildData.m_optionalBuildLogPath.IsValid() )
        {
            FileSystem::Path logPath = buildData.m_optionalBuildLogPath;
            logPath.ReplaceExtension( "bfx_log" );
            bfx::EnableBuildLog( logPath.c_str(), true );
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
}
#endif