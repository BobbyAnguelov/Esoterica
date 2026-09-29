#include "ResourceCompiler_RenderMesh.h"
#include "EngineTools/Resource/ResourceCompilerContext.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderMesh.h"
#include "EngineTools/Import/ImportedMesh.h"
#include "EngineTools/Import/Importer.h"
#include "Engine/Render/RenderMesh.h"
#include "Engine/Render/RenderGeometryBuilder.h"
#include "Base/Serialization/BinarySerialization.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    struct ConvertedMesh
    {
        TVector<GeometryBuilder>    m_geometryBuilders;
        bool                        m_isSkeletalMesh;
    };

    //-------------------------------------------------------------------------

    static ConvertedMesh ConvertMesh( Import::Mesh const& importedMesh )
    {
        // Copy mesh vertex data
        //-------------------------------------------------------------------------

        size_t totalVertexCount = 0;
        size_t totalTriangleCount = 0;

        ConvertedMesh convertedMesh = {};
        convertedMesh.m_isSkeletalMesh = importedMesh.IsSkeletalMesh();
        convertedMesh.m_geometryBuilders.resize( importedMesh.GetNumGeometries() );

        for ( size_t meshPartIndex = 0; meshPartIndex < importedMesh.GetNumGeometries(); ++meshPartIndex )
        {
            Import::Mesh::Geometry const& srcMeshPart = importedMesh.GetGeometries()[meshPartIndex];
            EE_ASSERT( srcMeshPart.m_numBoneInfluences <= 8 );

            GeometryBuilder& dstGeometryBuilder = convertedMesh.m_geometryBuilders[meshPartIndex];

            dstGeometryBuilder.SetNumTextureCoordinateAttributes( 2 );
            dstGeometryBuilder.SetNumColorAttributes( 1 );
            if ( importedMesh.IsSkeletalMesh() )
            {
                dstGeometryBuilder.SetNumSkinningAttributes( importedMesh.GetMaxNumberOfBoneInfluencesPerVertex() / 4 );
            }

            dstGeometryBuilder.InitializeVertexFormat();

            //-------------------------------------------------------------------------

            dstGeometryBuilder.SetIndices( srcMeshPart.m_indices, srcMeshPart.m_clockwiseWinding );
            dstGeometryBuilder.SetNumVertices( srcMeshPart.m_vertices.size() );

            for ( size_t vertexIndex = 0; vertexIndex < srcMeshPart.m_vertices.size(); ++vertexIndex )
            {
                auto const& srcVertex = srcMeshPart.m_vertices[vertexIndex];

                dstGeometryBuilder.SetPositionAttribute( vertexIndex, { srcVertex.m_position, srcVertex.m_normal } );

                GeometryBuilder::TextureCoordinateAttribute uv0 = ( srcMeshPart.GetNumUVChannels() > 0 ) ? srcVertex.m_texCoords[0] : Float2::Zero;
                GeometryBuilder::TextureCoordinateAttribute uv1 = ( srcMeshPart.GetNumUVChannels() > 1 ) ? srcVertex.m_texCoords[1] : Float2::Zero;

                dstGeometryBuilder.SetTextureCoordinateAttribute( vertexIndex, 0, uv0 );
                dstGeometryBuilder.SetTextureCoordinateAttribute( vertexIndex, 1, uv1 );

                // Bone influences and skeletal mesh data
                //-------------------------------------------------------------------------

                if ( importedMesh.IsSkeletalMesh() )
                {
                    int32_t const numInfluences = (int32_t) srcVertex.m_numBoneWeights;

                    if ( numInfluences > 0 )
                    {
                        GeometryBuilder::SkinningAttribute skinningAttribute0 = {};
                        skinningAttribute0.m_boneIndices = Int4( InvalidIndex, InvalidIndex, InvalidIndex, InvalidIndex );
                        skinningAttribute0.m_boneWeights = Float4::Zero;

                        for ( int32_t influenceIndex = 0; influenceIndex < Math::Min( numInfluences, 4 ); ++influenceIndex )
                        {
                            skinningAttribute0.m_boneIndices[influenceIndex] = srcVertex.m_boneIndices[influenceIndex];
                            skinningAttribute0.m_boneWeights[influenceIndex] = srcVertex.m_boneWeights[influenceIndex];
                        }

                        for ( uint32_t n = srcVertex.m_numBoneWeights; n < 4; n++ )
                        {
                            EE_ASSERT( skinningAttribute0.m_boneIndices[n] == -1 );
                        }

                        dstGeometryBuilder.SetSkinningAttribute( vertexIndex, 0, skinningAttribute0 );
                    }

                    //-------------------------------------------------------------------------

                    if ( numInfluences > 4 )
                    {
                        GeometryBuilder::SkinningAttribute skinningAttribute1 = {};
                        skinningAttribute1.m_boneIndices = Int4( InvalidIndex, InvalidIndex, InvalidIndex, InvalidIndex );
                        skinningAttribute1.m_boneWeights = Float4::Zero;

                        for ( int32_t influenceIndex = 4; influenceIndex < numInfluences; ++influenceIndex )
                        {
                            skinningAttribute1.m_boneIndices[influenceIndex - 4] = srcVertex.m_boneIndices[influenceIndex];
                            skinningAttribute1.m_boneWeights[influenceIndex - 4] = srcVertex.m_boneWeights[influenceIndex];
                        }

                        for ( uint32_t n = srcVertex.m_numBoneWeights; n < 4; n++ )
                        {
                            EE_ASSERT( skinningAttribute1.m_boneIndices[n] == -1 );
                        }

                        dstGeometryBuilder.SetSkinningAttribute( vertexIndex, 1, skinningAttribute1 );
                    }
                }
            }

            totalVertexCount += srcMeshPart.m_vertices.size();
            totalTriangleCount += srcMeshPart.GetNumTriangles();
        }

        EE_ASSERT( totalVertexCount < Geometry::s_maxMeshVertices );
        EE_ASSERT( totalTriangleCount < Geometry::s_maxMeshTriangles );

        for ( GeometryBuilder& geometryBuilder : convertedMesh.m_geometryBuilders )
        {
            geometryBuilder.Optimize();
        }

        return convertedMesh;
    }

    //-------------------------------------------------------------------------

    static MeshStatistics ComputeGeometryStats( Geometry const& geometry, uint64_t uncompressedSizeBytes )
    {
        MeshStatistics stats = {};

        stats.m_numVertices = geometry.GetNumVertices();
        stats.m_numTriangles = geometry.GetNumTriangles();
        stats.m_numClusters = geometry.GetNumClusters();

        stats.m_compressedSizeBytes = geometry.GetMemoryFootprint();
        stats.m_uncompressedSizeBytes = uncompressedSizeBytes;
        stats.m_compressionRatio = stats.m_uncompressedSizeBytes > 0 ? float( stats.m_compressedSizeBytes ) / float( stats.m_uncompressedSizeBytes ) : 0.0F;

        stats.m_vertexTriangleReuse = stats.m_numVertices > 0 ? float( stats.m_numTriangles ) * 3.0F / float( stats.m_numVertices ) : 0.0F;

        if ( stats.m_numClusters == 0 )
        {
            return stats;
        }

        TVector<float> clusterUtilizations;
        clusterUtilizations.reserve( stats.m_numClusters );

        uint64_t totalPositionBitsPerAxisX = 0;
        uint64_t totalPositionBitsPerAxisY = 0;
        uint64_t totalPositionBitsPerAxisZ = 0;
        uint64_t totalPositionBitsPerVertex = 0;

        TVector<uint32_t> bitsPerAxisX;
        TVector<uint32_t> bitsPerAxisY;
        TVector<uint32_t> bitsPerAxisZ;
        bitsPerAxisX.reserve( stats.m_numClusters );
        bitsPerAxisY.reserve( stats.m_numClusters );
        bitsPerAxisZ.reserve( stats.m_numClusters );

        stats.m_minimumPositionBitsPerVertex = UINT32_MAX;
        stats.m_minimumClusterUtilization = 1.0F;
        stats.m_minimumCompressionAccuracy = 1.0F;

        double totalClusterUtilization = 0.0;
        double totalCompressionAccuracy = 0.0;

        for ( uint32_t clusterIndex = 0; clusterIndex < stats.m_numClusters; ++clusterIndex )
        {
            MeshCluster const& cluster = geometry.GetCluster( clusterIndex );

            // Position bits
            //-------------------------------------------------------------------------

            uint32_t const bitsX = cluster.GetNumPositionBitsX();
            uint32_t const bitsY = cluster.GetNumPositionBitsY();
            uint32_t const bitsZ = cluster.GetNumPositionBitsZ();
            uint32_t const bitsPerVertex = bitsX + bitsY + bitsZ;

            totalPositionBitsPerAxisX += bitsX;
            totalPositionBitsPerAxisY += bitsY;
            totalPositionBitsPerAxisZ += bitsZ;
            totalPositionBitsPerVertex += bitsPerVertex;

            bitsPerAxisX.push_back( bitsX );
            bitsPerAxisY.push_back( bitsY );
            bitsPerAxisZ.push_back( bitsZ );

            stats.m_minimumPositionBitsPerVertex = Math::Min( stats.m_minimumPositionBitsPerVertex, bitsPerVertex );
            stats.m_maximumPositionBitsPerVertex = Math::Max( stats.m_maximumPositionBitsPerVertex, bitsPerVertex );

            // Cluster utilization ( vertex slots )
            //-------------------------------------------------------------------------

            float const utilization = float( cluster.GetNumVertices() ) / float( MeshCluster::s_maxVerticesPerCluster );
            clusterUtilizations.push_back( utilization );
            stats.m_minimumClusterUtilization = Math::Min( stats.m_minimumClusterUtilization, utilization );
            stats.m_maximumClusterUtilization = Math::Max( stats.m_maximumClusterUtilization, utilization );
            totalClusterUtilization += utilization;

            // Compression accuracy - quantization error vs cluster extent
            //-------------------------------------------------------------------------

            Float3 aabbMin, aabbMax;
            cluster.GetAABB( aabbMin, aabbMax );

            float const dx = aabbMax.m_x - aabbMin.m_x;
            float const dy = aabbMax.m_y - aabbMin.m_y;
            float const dz = aabbMax.m_z - aabbMin.m_z;
            float const clusterExtent = Math::Sqrt( dx * dx + dy * dy + dz * dz );
            float const quantizationError = ldexpf( 0.5F, cluster.GetSharedExponent() );
            float const accuracy = clusterExtent > 0.0F ? Math::Max( 0.0F, 1.0F - quantizationError / clusterExtent ) : 1.0F;
            stats.m_minimumCompressionAccuracy = Math::Min( stats.m_minimumCompressionAccuracy, accuracy );
            totalCompressionAccuracy += accuracy;
        }

        float const numClustersF = float( stats.m_numClusters );

        stats.m_averagePositionBitsPerAxisX = float( totalPositionBitsPerAxisX ) / numClustersF;
        stats.m_averagePositionBitsPerAxisY = float( totalPositionBitsPerAxisY ) / numClustersF;
        stats.m_averagePositionBitsPerAxisZ = float( totalPositionBitsPerAxisZ ) / numClustersF;
        stats.m_averagePositionBitsPerVertex = float( totalPositionBitsPerVertex ) / numClustersF;

        eastl::sort( bitsPerAxisX.begin(), bitsPerAxisX.end() );
        eastl::sort( bitsPerAxisY.begin(), bitsPerAxisY.end() );
        eastl::sort( bitsPerAxisZ.begin(), bitsPerAxisZ.end() );

        uint32_t const medianClusterIndex = stats.m_numClusters / 2;

        stats.m_minimumPositionBitsPerAxisX = bitsPerAxisX[0];
        stats.m_minimumPositionBitsPerAxisY = bitsPerAxisY[0];
        stats.m_minimumPositionBitsPerAxisZ = bitsPerAxisZ[0];

        stats.m_medianPositionBitsPerAxisX = float( bitsPerAxisX[medianClusterIndex] );
        stats.m_medianPositionBitsPerAxisY = float( bitsPerAxisY[medianClusterIndex] );
        stats.m_medianPositionBitsPerAxisZ = float( bitsPerAxisZ[medianClusterIndex] );

        stats.m_maximumPositionBitsPerAxisX = bitsPerAxisX[stats.m_numClusters - 1];
        stats.m_maximumPositionBitsPerAxisY = bitsPerAxisY[stats.m_numClusters - 1];
        stats.m_maximumPositionBitsPerAxisZ = bitsPerAxisZ[stats.m_numClusters - 1];

        stats.m_averageClusterUtilization = float( totalClusterUtilization / double( stats.m_numClusters ) );
        stats.m_clusterVertexOverhead = 1.0F - stats.m_averageClusterUtilization;

        eastl::sort( clusterUtilizations.begin(), clusterUtilizations.end() );
        stats.m_medianClusterUtilization = clusterUtilizations[stats.m_numClusters / 2];

        stats.m_averageCompressionAccuracy = float( totalCompressionAccuracy / double( stats.m_numClusters ) );

        return stats;
    }

    //-------------------------------------------------------------------------

    Resource::CompilationResult MeshCompiler::CompileMesh( Resource::CompileContext const& ctx, Mesh& mesh, MeshResourceDescriptor const& resourceDescriptor, MeshGroup const& meshGroup ) const
    {
        bool hasWarnings = false;

        bool const isSkeletalMesh = resourceDescriptor.GetCompiledResourceTypeID() == SkeletalMesh::GetStaticResourceTypeID();

        Import::ReaderContext readerCtx =
        {
            [&ctx]( char const* pString ) { ctx.LogWarning( pString ); },
            [&ctx] ( char const* pString ) { ctx.LogError( pString ); },
        };

        DataPath const meshDataPath = resourceDescriptor.m_meshPath;
        DataPath const meshPathDir = meshDataPath.GetParentDirectory();
        FileSystem::Extension const extension = meshDataPath.GetExtension();
        String const meshFilename = resourceDescriptor.m_meshPath.GetFilenameWithoutExtension();

        // LOD
        //-------------------------------------------------------------------------

        if ( meshGroup.m_lodSettings.size() > 8 )
        {
            return ctx.LogError( "Only 8 LODs are supported at the moment" );
        }

        // LODs
        AABB combinedAABB;

        for ( MeshLODSettings const& lod : meshGroup.m_lodSettings )
        {
            uint32_t const lodGeometryBaseIndex = uint32_t( mesh.m_geometry.size() );

            // Do we have an explicit LOD mesh specified, if so, use that
            DataPath lodMeshDataPath;
            if ( !lod.m_filenameSuffix.empty() )
            {
                lodMeshDataPath = DataPath( String( String::CtorSprintf(), "%s%s%s.%s", meshPathDir.c_str(), meshFilename.c_str(), lod.m_filenameSuffix.c_str(), extension.c_str() ) );
            }
            else
            {
                lodMeshDataPath = meshDataPath;
            }

            // TODO: Don't load the same mesh multiple times!
            FileSystem::Path const lodMeshFilePath = lodMeshDataPath.GetFileSystemPath( ctx.m_sourceResourceDirectoryPath );
            Import::Source const fileSource( lodMeshFilePath, ctx.GetRawData( lodMeshDataPath ) );

            TUniquePtr<Import::Mesh> importedMesh;
            if ( isSkeletalMesh )
            {
                importedMesh = Import::Importer::ReadSkeletalMesh( readerCtx, fileSource, resourceDescriptor.m_meshesToInclude );
            }
            else
            {
                importedMesh = Import::Importer::ReadStaticMesh( readerCtx, fileSource, resourceDescriptor.m_meshesToInclude );
            }

            if ( !importedMesh || !importedMesh->IsValid() )
            {
                return ctx.LogError( "Failed to import source file: %s", lodMeshFilePath.c_str() );
            }

            if ( importedMesh->IsSkeletalMesh() && importedMesh->GetMaxNumberOfBoneInfluencesPerVertex() > 8 )
            {
                return ctx.LogError( "More than 8 bone influences detected - this is unsupported" );
            }

            // Setup bones and bind pose
            //-------------------------------------------------------------------------

            if ( &lod == meshGroup.m_lodSettings.begin() ) // TODO: Didn't explode somehow
            {
                SkeletalMesh* pSkeletalMesh = nullptr;

                if ( isSkeletalMesh )
                {
                    EE_ASSERT( importedMesh );
                    EE_ASSERT( importedMesh->IsSkeletalMesh() );

                    pSkeletalMesh = &static_cast<SkeletalMesh&>( mesh );

                    auto const& skeleton = importedMesh->GetSkeleton();
                    auto const& boneData = skeleton.GetBoneData();

                    auto const numBones = skeleton.GetNumBones();
                    for ( size_t boneIndex = 0; boneIndex < numBones; ++boneIndex )
                    {
                        pSkeletalMesh->m_boneIDs.push_back( boneData[boneIndex].m_name );
                        pSkeletalMesh->m_parentBoneIndices.push_back( boneData[boneIndex].m_parentBoneIdx );
                        pSkeletalMesh->m_parentSpaceBindPose.push_back( boneData[boneIndex].m_parentSpaceTransform );
                        pSkeletalMesh->m_modelSpaceBindPose.push_back( boneData[boneIndex].m_modelSpaceTransform );
                        pSkeletalMesh->m_modelSpaceInverseBindPose.push_back( boneData[boneIndex].m_modelSpaceTransform.GetInverse() );
                    }
                }
            }

            hasWarnings = hasWarnings || importedMesh->HasWarnings();

            //-------------------------------------------------------------------------

            ConvertedMesh convertedMesh = ConvertMesh( *importedMesh );

            // Validate vertex attributes, this is a temporary restriction that will be fixed soon
            //-------------------------------------------------------------------------

            for ( GeometryBuilder const& geometryBuilder : convertedMesh.m_geometryBuilders )
            {
                if ( geometryBuilder.GetNumSkinningAttributes() != convertedMesh.m_geometryBuilders[0].GetNumSkinningAttributes() )
                {
                    return ctx.LogError( "Imported mesh has different amount of skinning attributes per geometry, this is not supported yet." );
                }
            }

            // Generate LOD
            //-------------------------------------------------------------------------

            bool isValidMeshData = false;
            TVector<MeshStatistics> lodGeometryStats;

            for ( GeometryBuilder const& geometryBuilder : convertedMesh.m_geometryBuilders )
            {
                if ( geometryBuilder.GetVertices().empty() || geometryBuilder.GetIndices().empty() )
                {
                    ctx.LogWarning( "Empty geometry skipped" );
                    continue;
                }

                Geometry geometry = {};
                uint64_t geometryUncompressedSizeBytes = 0;

                if ( lod.m_autoGenerateLOD )
                {
                    GeometryBuilder lodGeometryBuilder = geometryBuilder;

                    // TODO: Handle individual simplification status. Currently if at least 1 mesh simplification succeeded we copy the entire data set even if others failed.
                    bool simplificationSuccess = lodGeometryBuilder.Simplify
                    (
                        lod.m_targetAttributeWeightNormals,
                        lod.m_targetAttributeWeightUV,
                        lod.m_targetAttributeMaxError,
                        lod.m_targetTriangleCount,
                        lod.m_targetTrianglePercentage
                    );

                    if ( simplificationSuccess || &lod == meshGroup.m_lodSettings.begin() )
                    {
                        lodGeometryBuilder.Optimize();
                        geometry = lodGeometryBuilder.BuildGeometry();
                        geometryUncompressedSizeBytes = uint64_t( lodGeometryBuilder.GetVertices().size() ) + uint64_t( lodGeometryBuilder.GetIndices().size() ) * 4;

                        isValidMeshData = true;

                        EE_ASSERT( !geometry.GetMeshData().empty() );
                    }
                }
                else
                {
                    geometry = geometryBuilder.BuildGeometry();
                    geometryUncompressedSizeBytes = uint64_t( geometryBuilder.GetVertices().size() ) + uint64_t( geometryBuilder.GetIndices().size() ) * 4;

                    isValidMeshData = true;

                    EE_ASSERT( !geometry.GetMeshData().empty() );

                }

                if ( isValidMeshData )
                {
                    if ( geometry.GetMeshData().empty() )
                    {
                        continue;
                    }

                    lodGeometryStats.emplace_back( ComputeGeometryStats( geometry, geometryUncompressedSizeBytes ) );
                    mesh.m_geometry.emplace_back( eastl::move( geometry ) );
                }
            }

            //-------------------------------------------------------------------------

            if ( isValidMeshData )
            {
                uint32_t geometryIndex = 0;
                for ( Import::Mesh::Submesh const& importedSubmesh : importedMesh->GetSubmeshes() )
                {
                    // Need to account for empty geometries that are skipped earlier
                    GeometryBuilder const& sourceGeometryBuilder = convertedMesh.m_geometryBuilders[geometryIndex];
                    if ( sourceGeometryBuilder.GetVertices().empty() || sourceGeometryBuilder.GetIndices().empty() )
                    {
                        continue;
                    }

                    Geometry const& geometry = mesh.m_geometry[lodGeometryBaseIndex + geometryIndex];
                    if ( geometry.GetMeshData().empty() )
                    {
                        continue;
                    }

                    Mesh::Submesh submesh = {};
                    submesh.m_ID = importedSubmesh.m_ID;
                    submesh.m_materialNameID = importedSubmesh.m_materialID;
                    submesh.m_geometryIdx = lodGeometryBaseIndex + geometryIndex;
                    submesh.m_lodMask = uint8_t( 1 ) << uint8_t( mesh.m_geometryLODDistance.size() );

                    mesh.m_submeshes.emplace_back( eastl::move( submesh ) );
                    mesh.m_submeshLocalTransforms.emplace_back( Matrix43( importedSubmesh.m_transform ) );

                    AABB transformedAABB = geometry.GetBounds().GetAABB().GetTransformed( importedSubmesh.m_transform );

                    if ( !combinedAABB.IsValid() )
                    {
                        combinedAABB = transformedAABB;
                    }
                    else
                    {
                        combinedAABB = AABB::GetCombinedBox( transformedAABB, combinedAABB );
                    }

                    geometryIndex++;
                }

                MeshStatistics lodStatistics = {};
                for ( MeshStatistics const& geometryStats : lodGeometryStats )
                {
                    lodStatistics.Accumulate( geometryStats );
                }
                mesh.m_statisticsPerLOD.push_back( lodStatistics );
                mesh.m_geometryLODDistance.push_back( lod.m_lodDistance );
            }
        }

        mesh.m_meshBounds = OBB( combinedAABB );
        mesh.m_numLODs = uint32_t( mesh.m_geometryLODDistance.size() );

        // Resolve material mappings
        //-------------------------------------------------------------------------

        for ( Mesh::Submesh& submesh : mesh.m_submeshes )
        {
            StringID const submeshMaterialMappingID = MeshMaterialMapping::GetMappingID( submesh );

            for ( MeshMaterialMapping const& mapping : resourceDescriptor.m_materialMappings )
            {
                if ( submeshMaterialMappingID == mapping.m_mappingID )
                {
                    submesh.m_material = mapping.m_material;
                }
            }

            if ( !submesh.m_material.IsSet() )
            {
                submesh.m_material = ResourceID( "data://Render/Materials/PlaceholderMaterial.material" );
                ctx.LogWarning( "Could not resolve material slot %s, setting placeholder material", submesh.m_ID.c_str() );
            }
        }

        // Setup Sockets
        //-------------------------------------------------------------------------

        for ( auto const& socketDef : resourceDescriptor.m_sockets )
        {
            if ( !socketDef.m_ID.IsValid() )
            {
                ctx.LogWarning( "Ignoring Socket: Invalid socket ID" );
                continue;
            }

            if ( mesh.GetSocketIndex( socketDef.m_ID ) != InvalidIndex )
            {
                ctx.LogWarning( "Ignoring Socket: Duplicate socket ID encountered (%s)", socketDef.m_ID.c_str() );
                continue;
            }

            if ( isSkeletalMesh )
            {
                SkeletalMesh* pSkeletalMesh = static_cast<SkeletalMesh*>( &mesh );
                if ( pSkeletalMesh->GetBoneIndex( socketDef.m_ID ) != InvalidIndex )
                {
                    ctx.LogWarning( "Ignoring Socket: Socket ID is not allowed to match a bone ID (%s)", socketDef.m_ID.c_str() );
                    continue;
                }
            }

            int32_t boneIdx = InvalidIndex;
            if ( socketDef.m_boneID.IsValid() )
            {
                SkeletalMesh* pSkeletalMesh = static_cast<SkeletalMesh*>( &mesh );
                if ( isSkeletalMesh )
                {
                    boneIdx = pSkeletalMesh->GetBoneIndex( socketDef.m_boneID );
                    if ( boneIdx == InvalidIndex )
                    {
                        ctx.LogWarning( "Ignoring Socket: Invalid bone ID specified for socket (%s)", socketDef.m_boneID.c_str() );
                        continue;
                    }
                }
                else
                {
                    ctx.LogWarning( "Ignoring Socket: Socket has a specified bone ID (%s) for a static mesh", socketDef.m_boneID.c_str() );
                    continue;
                }
            }

            auto& socket = mesh.m_sockets.emplace_back();
            socket.m_ID = socketDef.m_ID;
            socket.m_boneIdx = boneIdx;
            socket.m_offset = socketDef.m_offsetTransform;
        }

        //-------------------------------------------------------------------------

        if ( hasWarnings )
        {
            return Resource::CompilationResult::SuccessWithWarnings;
        }

        return Resource::CompilationResult::Success;
    }

    void MeshCompiler::SetResourceHeaderInstallDependencies( Mesh const& mesh, Resource::ResourceHeader& hdr )
    {
        for ( Mesh::Submesh const& submesh : mesh.m_submeshes )
        {
            if ( submesh.m_material.IsSet() )
            {
                hdr.AddInstallDependency( submesh.m_material.GetResourceID() );
            }
        }
    }

    //-------------------------------------------------------------------------

    StaticMeshCompiler::StaticMeshCompiler()
        : MeshCompiler( "StaticMeshCompiler" )
    {
        RegisterOutput<StaticMesh>();
    }

    Resource::CompilationResult StaticMeshCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        auto pResourceDescriptor = ctx.GetDescriptor<StaticMeshResourceDescriptor>();

        if ( !pResourceDescriptor->m_meshGroup.IsValid() )
        {
            return ctx.LogError( "There is no mesh group set" );
        }

        if ( pResourceDescriptor->m_materialMappings.empty() )
        {
            ctx.LogWarning( "There are no material mappings set" );
        }

        // Reflect FBX data into runtime format
        //-------------------------------------------------------------------------

        StaticMesh staticMesh = {};

        auto pMeshGroup = ctx.GetDataFile<MeshGroup>( pResourceDescriptor->m_meshGroup );
        Resource::CompilationResult result = CompileMesh( ctx, staticMesh, *pResourceDescriptor, *pMeshGroup );
        if ( result == Resource::CompilationResult::Failure )
        {
            return result;
        }

        // Serialize
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( StaticMesh::s_version, StaticMesh::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );
        SetResourceHeaderInstallDependencies( staticMesh, hdr );

        Serialization::BinaryOutputArchive archive;
        archive << hdr << staticMesh;

        if ( archive.WriteToFile( ctx.GetOutputPath() ) )
        {
            return result;
        }
        else
        {
            return Resource::CompilationResult::Failure;
        }
    }

    //-------------------------------------------------------------------------

    SkeletalMeshCompiler::SkeletalMeshCompiler()
        : MeshCompiler( "SkeletalMeshCompiler" )
    {
        RegisterOutput<SkeletalMesh>();
    }

    Resource::CompilationResult SkeletalMeshCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        auto pResourceDescriptor = ctx.GetDescriptor<SkeletalMeshResourceDescriptor>();

        if ( !pResourceDescriptor->m_meshGroup.IsValid() )
        {
            return ctx.LogError( "There is no mesh group set" );
        }

        // Reflect FBX data into runtime format
        //-------------------------------------------------------------------------

        SkeletalMesh skeletalMesh = {};

        auto pMeshGroup = ctx.GetDataFile<MeshGroup>( pResourceDescriptor->m_meshGroup );
        Resource::CompilationResult result = CompileMesh( ctx, skeletalMesh, *pResourceDescriptor, *pMeshGroup );
        if ( result == Resource::CompilationResult::Failure )
        {
            return result;
        }

        // Serialize
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( StaticMesh::s_version, SkeletalMesh::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );
        SetResourceHeaderInstallDependencies( skeletalMesh, hdr );

        Serialization::BinaryOutputArchive archive;
        archive << hdr << skeletalMesh;

        if ( archive.WriteToFile( ctx.GetOutputPath() ) )
        {
            return result;
        }
        return Resource::CompilationResult::Failure;
    }
}
