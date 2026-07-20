#include "ResourceCompiler_RenderMesh.h"
#include "EngineTools/Resource/ResourceCompilerContext.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderMesh.h"
#include "EngineTools/Import/ImportedMesh.h"
#include "EngineTools/Import/Importer.h"
#include "Engine/Render/RenderMesh.h"
#include "Engine/Render/RenderGeometryBuilder.h"
#include "Base/Serialization/BinarySerialization.h"

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

            //---------------------------------------

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
                //-----------------------------------------------------------------------------------------------------

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

                        for ( int32_t influenceIndex = 0; influenceIndex < Math::Min( numInfluences - 4, 0 ); ++influenceIndex )
                        {
                            skinningAttribute1.m_boneIndices[influenceIndex] = srcVertex.m_boneIndices[4 + influenceIndex];
                            skinningAttribute1.m_boneWeights[influenceIndex] = srcVertex.m_boneWeights[4 + influenceIndex];
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

        EE_ASSERT( totalVertexCount < Geometry::MaxMeshVertices );
        EE_ASSERT( totalTriangleCount < Geometry::MaxMeshTriangles );

        for ( GeometryBuilder& geometryBuilder : convertedMesh.m_geometryBuilders )
        {
            geometryBuilder.Optimize();
        }

        return convertedMesh;
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
                        pSkeletalMesh->GetBoneIDs().push_back( boneData[boneIndex].m_name );
                        pSkeletalMesh->GetParentBoneIndices().push_back( boneData[boneIndex].m_parentBoneIdx );
                        pSkeletalMesh->GetBindPose().push_back( boneData[boneIndex].m_modelSpaceTransform );
                        pSkeletalMesh->GetInverseBindPose().push_back( boneData[boneIndex].m_modelSpaceTransform.GetInverse() );
                    }
                }
            }

            hasWarnings = hasWarnings || importedMesh->HasWarnings();

            //-------------------------------------------------------------------------

            ConvertedMesh convertedMesh = ConvertMesh( *importedMesh );

            // Generate LOD
            //-------------------------------------------------------------------------

            bool isValidMeshData = false;

            for ( GeometryBuilder const& geometryBuilder : convertedMesh.m_geometryBuilders )
            {
                Geometry geometry = {};
                geometry.SetVertexStride( uint32_t( convertedMesh.m_isSkeletalMesh ? sizeof( SkeletalMeshVertex ) : sizeof( StaticMeshVertex ) ) );

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
                        lodGeometryBuilder.BuildAndAppendGeometry( geometry );
                        geometry.SetBounds( OBB( lodGeometryBuilder.ComputeAABB() ) ); // TODO: use real algorithm to find minimal bounding box, for now use AABB

                        isValidMeshData = true;
                    }
                }
                else
                {
                    geometryBuilder.BuildAndAppendGeometry( geometry );
                    geometry.SetBounds( OBB( geometryBuilder.ComputeAABB() ) ); // TODO: use real algorithm to find minimal bounding box, for now use AABB

                    isValidMeshData = true;
                }

                if ( isValidMeshData )
                {
                    mesh.m_geometry.emplace_back( eastl::move( geometry ) );
                }
            }

            //-------------------------------------------------------------------------

            if ( isValidMeshData )
            {
                for ( Import::Mesh::Submesh const& importedSubmesh : importedMesh->GetSubmeshes() )
                {
                    Mesh::Submesh submesh = {};
                    submesh.m_ID = importedSubmesh.m_ID;
                    submesh.m_materialNameID = importedSubmesh.m_materialID;
                    submesh.m_geometryIdx = uint32_t( mesh.m_geometryLODDistance.size() + importedSubmesh.m_geometryIdx );
                    submesh.m_lodMask = uint8_t( 1 ) << uint8_t( mesh.m_geometryLODDistance.size() );

                    mesh.m_submeshes.emplace_back( eastl::move( submesh ) );
                    mesh.m_submeshLocalTransforms.emplace_back( Matrix43( importedSubmesh.m_transform ) );

                    Geometry const& geo = mesh.m_geometry[submesh.m_geometryIdx];
                    AABB transformedAABB = geo.GetBounds().GetAABB().GetTransformed( importedSubmesh.m_transform );

                    if ( !combinedAABB.IsValid() )
                    {
                        combinedAABB = transformedAABB;
                    }
                    else
                    {
                        combinedAABB = AABB::GetCombinedBox( transformedAABB, combinedAABB );
                    }
                }

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
            return ctx.LogError( "There are no material mappings set" );
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
