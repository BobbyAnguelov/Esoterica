#include "ResourceCompiler_RenderMesh.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderMesh.h"
#include "EngineTools/RawAssets/RawMesh.h"
#include "EngineTools/RawAssets/RawAssetReader.h"
#include "Engine/Render/Mesh/StaticMesh.h"
#include "Engine/Render/Mesh/SkeletalMesh.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/Serialization/BinarySerialization.h"

#include <MeshOptimizer.h>

//-------------------------------------------------------------------------

namespace EE::Render
{
    void MeshCompiler::TransferMeshGeometry( RawAssets::RawMesh const& rawMesh, Mesh& mesh, int32_t maxBoneInfluences ) const
    {
        EE_ASSERT( maxBoneInfluences > 0 && maxBoneInfluences <= 8 );
        EE_ASSERT( maxBoneInfluences <= 4 );// TEMP HACK - we dont support 8 bones for now

        // Merge all mesh geometries into the main vertex and index buffers
        //-------------------------------------------------------------------------

        uint32_t numVertices = 0;
        uint32_t numIndices = 0;

        for ( auto const& geometrySection : rawMesh.GetGeometrySections() )
        {
            // Add sub-mesh record
            mesh.m_sections.push_back( Mesh::GeometrySection( StringID( geometrySection.m_name ), numIndices, (uint32_t) geometrySection.m_indices.size() ) );

            for ( auto idx : geometrySection.m_indices )
            {
                mesh.m_indices.push_back( numVertices + idx );
            }

            numIndices += (uint32_t) geometrySection.m_indices.size();
            numVertices += (uint32_t) geometrySection.m_vertices.size();
        }

        // Copy mesh vertex data
        //-------------------------------------------------------------------------

        AABB meshAlignedBounds;
        int32_t vertexSize = 0;
        int32_t vertexBufferSize = 0;

        if ( rawMesh.IsSkeletalMesh() )
        {
            mesh.m_vertexBuffer.m_vertexFormat = VertexFormat::SkeletalMesh;
            vertexSize = VertexLayoutRegistry::GetDescriptorForFormat( mesh.m_vertexBuffer.m_vertexFormat ).m_byteSize;
            EE_ASSERT( vertexSize == sizeof( SkeletalMeshVertex ) );

            vertexBufferSize = vertexSize * numVertices;
            mesh.m_vertices.resize( vertexBufferSize );
            auto pVertexMemory = (SkeletalMeshVertex*) mesh.m_vertices.data();

            for ( auto const& geometrySection : rawMesh.GetGeometrySections() )
            {
                for ( auto const& vert : geometrySection.m_vertices )
                {
                    auto pVertex = new( pVertexMemory ) SkeletalMeshVertex();

                    pVertex->m_position = vert.m_position;
                    pVertex->m_normal = vert.m_normal;
                    pVertex->m_UV0 = vert.m_texCoords[0];
                    pVertex->m_UV1 = ( geometrySection.GetNumUVChannels() > 1 ) ? vert.m_texCoords[1] : vert.m_texCoords[0];

                    int32_t const numInfluences = (int32_t) vert.m_boneIndices.size();
                    EE_ASSERT( numInfluences <= maxBoneInfluences && vert.m_boneIndices.size() == vert.m_boneWeights.size() );

                    pVertex->m_boneIndices = Int4( InvalidIndex, InvalidIndex, InvalidIndex, InvalidIndex );
                    pVertex->m_boneWeights = Float4::Zero;

                    int32_t const numWeights = Math::Min( numInfluences, 4 );
                    for ( int32_t i = 0; i < numWeights; i++ )
                    {
                        pVertex->m_boneIndices[i] = vert.m_boneIndices[i];
                        pVertex->m_boneWeights[i] = vert.m_boneWeights[i];
                    }

                    // Re-enable this when we add back support for 8 bone weights
                    /*pVertex->m_boneIndices1 = Int4( InvalidIndex, InvalidIndex, InvalidIndex, InvalidIndex );
                    pVertex->m_boneWeights1 = Float4::Zero;
                    for ( int32_t i = 4; i < numInfluences; i++ )
                    {
                        pVertex->m_boneIndices1[i - 4] = vert.m_boneIndices[i];
                        pVertex->m_boneWeights1[i - 4] = vert.m_boneWeights[i];
                    }*/

                    pVertexMemory++;

                    //-------------------------------------------------------------------------

                    meshAlignedBounds.AddPoint( vert.m_position );
                }
            }
        }
        else
        {
            mesh.m_vertexBuffer.m_vertexFormat = VertexFormat::StaticMesh;
            vertexSize = VertexLayoutRegistry::GetDescriptorForFormat( mesh.m_vertexBuffer.m_vertexFormat ).m_byteSize;
            EE_ASSERT( vertexSize == sizeof( StaticMeshVertex ) );

            vertexBufferSize = vertexSize * numVertices;
            mesh.m_vertices.resize( vertexBufferSize );
            auto pVertexMemory = (StaticMeshVertex*) mesh.m_vertices.data();

            for ( auto const& geometrySection : rawMesh.GetGeometrySections() )
            {
                for ( auto const& vert : geometrySection.m_vertices )
                {
                    auto pVertex = new( pVertexMemory ) StaticMeshVertex();

                    pVertex->m_position = vert.m_position;
                    pVertex->m_normal = vert.m_normal;
                    pVertex->m_UV0 = vert.m_texCoords[0];
                    pVertex->m_UV1 = ( geometrySection.GetNumUVChannels() > 1 ) ? vert.m_texCoords[1] : vert.m_texCoords[0];

                    pVertexMemory++;

                    //-------------------------------------------------------------------------

                    meshAlignedBounds.AddPoint( vert.m_position );
                }
            }
        }

        // Set Mesh buffer descriptors
        //-------------------------------------------------------------------------

        mesh.m_vertexBuffer.m_byteStride = vertexSize;
        mesh.m_vertexBuffer.m_byteSize = vertexBufferSize;
        mesh.m_vertexBuffer.m_type = RenderBuffer::Type::Vertex;
        mesh.m_vertexBuffer.m_usage = RenderBuffer::Usage::GPU_only;

        mesh.m_indexBuffer.m_byteStride = sizeof( uint32_t );
        mesh.m_indexBuffer.m_byteSize = (uint32_t) mesh.m_indices.size() * sizeof( uint32_t );
        mesh.m_indexBuffer.m_type = RenderBuffer::Type::Index;
        mesh.m_indexBuffer.m_usage = RenderBuffer::Usage::GPU_only;

        // Calculate bounding volume
        //-------------------------------------------------------------------------
        // TODO: use real algorithm to find minimal bounding box, for now use AABB

        mesh.m_bounds = OBB( meshAlignedBounds );
    }

    void MeshCompiler::OptimizeMeshGeometry( Mesh& mesh ) const
    {
        size_t const vertexSize = (size_t) mesh.m_vertexBuffer.m_byteStride;
        size_t const numVertices = (size_t) mesh.GetNumVertices();

        for ( auto const& section : mesh.m_sections )
        {
            meshopt_optimizeVertexCache( &mesh.m_indices[section.m_startIndex], &mesh.m_indices[section.m_startIndex], section.m_numIndices, mesh.m_vertices.size() );

            // Reorder indices for overdraw, balancing overdraw and vertex cache efficiency
            const float kThreshold = 1.01f; // allow up to 1% worse ACMR to get more reordering opportunities for overdraw
            meshopt_optimizeOverdraw( &mesh.m_indices[section.m_startIndex], &mesh.m_indices[section.m_startIndex], section.m_numIndices, (float*) &mesh.m_vertices[0], numVertices, vertexSize, kThreshold );
        }

        // Vertex fetch optimization should go last as it depends on the final index order
        meshopt_optimizeVertexFetch( &mesh.m_vertices[0], &mesh.m_indices[0], mesh.m_indices.size(), &mesh.m_vertices[0], numVertices, vertexSize );
    }

    void MeshCompiler::SetMeshDefaultMaterials( MeshResourceDescriptor const& descriptor, Mesh& mesh ) const
    {
        mesh.m_materials.reserve( mesh.GetNumSections() );

        for ( auto i = 0u; i < mesh.GetNumSections(); i++ )
        {
            if ( i < descriptor.m_materials.size() )
            {
                mesh.m_materials.push_back( descriptor.m_materials[i] );
            }
            else
            {
                mesh.m_materials.push_back( TResourcePtr<Material>() );
            }
        }
    }

    void MeshCompiler::SetMeshInstallDependencies( Mesh const& mesh, Resource::ResourceHeader& hdr ) const
    {
        for ( auto i = 0u; i < mesh.m_materials.size(); i++ )
        {
            if ( mesh.m_materials[i].IsSet() )
            {
                hdr.AddInstallDependency( mesh.m_materials[i].GetResourceID() );
            }
        }
    }

    bool MeshCompiler::GetInstallDependencies( ResourceID const& resourceID, TVector<ResourceID>& outReferencedResources ) const
    {
        FileSystem::Path const filePath = resourceID.GetResourcePath().ToFileSystemPath( m_rawResourceDirectoryPath );
        MeshResourceDescriptor resourceDescriptor;
        if ( !Resource::ResourceDescriptor::TryReadFromFile( *m_pTypeRegistry, filePath, resourceDescriptor ) )
        {
            Error( "Failed to read resource descriptor from input file: %s", filePath.c_str() );
            return false;
        }

        for ( auto const& materialResourceID : resourceDescriptor.m_materials )
        {
            if ( materialResourceID.GetResourceID().IsValid() )
            {
                VectorEmplaceBackUnique( outReferencedResources, materialResourceID.GetResourceID() );
            }
        }

        return true;
    }

    //-------------------------------------------------------------------------

    StaticMeshCompiler::StaticMeshCompiler()
        : MeshCompiler( "StaticMeshCompiler", s_version )
    {
        m_outputTypes.push_back( StaticMesh::GetStaticResourceTypeID() );
    }

    Resource::CompilationResult StaticMeshCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        StaticMeshResourceDescriptor resourceDescriptor;
        if ( !Resource::ResourceDescriptor::TryReadFromFile( *m_pTypeRegistry, ctx.m_inputFilePath, resourceDescriptor ) )
        {
            return Error( "Failed to read resource descriptor from input file: %s", ctx.m_inputFilePath.c_str() );
        }

        if ( Math::IsNearZero( resourceDescriptor.m_scale.m_x ) || Math::IsNearZero( resourceDescriptor.m_scale.m_y ) || Math::IsNearZero( resourceDescriptor.m_scale.m_z ) )
        {
            return Error( "Zero Scale is not allowed!", ctx.m_inputFilePath.c_str() );
        }

        // Read mesh data
        //-------------------------------------------------------------------------

        FileSystem::Path meshFilePath;
        if ( !ConvertResourcePathToFilePath( resourceDescriptor.m_meshPath, meshFilePath ) )
        {
            return Error( "Invalid mesh data path: %s", resourceDescriptor.m_meshPath.c_str() );
        }

        RawAssets::ReaderContext readerCtx = { [this]( char const* pString ) { Warning( pString ); }, [this] ( char const* pString ) { Error( pString ); } };
        TUniquePtr<RawAssets::RawMesh> pRawMesh = RawAssets::ReadStaticMesh( readerCtx, meshFilePath, resourceDescriptor.m_meshesToInclude );
        if ( pRawMesh == nullptr )
        {
            return Error( "Failed to read mesh from source file" );
        }

        EE_ASSERT( pRawMesh->IsValid() );

        pRawMesh->ApplyScale( resourceDescriptor.m_scale );

        if ( resourceDescriptor.m_mergeSectionsByMaterial )
        {
            pRawMesh->MergeGeometrySectionsByMaterial();
        }

        // Reflect FBX data into runtime format
        //-------------------------------------------------------------------------

        StaticMesh staticMesh;

        TransferMeshGeometry( *pRawMesh, staticMesh, 4 );
        OptimizeMeshGeometry( staticMesh );
        SetMeshDefaultMaterials( resourceDescriptor, staticMesh );

        // Serialize
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( s_version, StaticMesh::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );
        SetMeshInstallDependencies( staticMesh, hdr );

        Serialization::BinaryOutputArchive archive;
        archive << hdr << staticMesh;

        if ( archive.WriteToFile( ctx.m_outputFilePath ) )
        {
            if ( pRawMesh->HasWarnings() )
            {
                return CompilationSucceededWithWarnings( ctx );
            }
            else
            {
                return CompilationSucceeded( ctx );
            }
        }
        else
        {
            return CompilationFailed( ctx );
        }
    }

    //-------------------------------------------------------------------------

    SkeletalMeshCompiler::SkeletalMeshCompiler()
        : MeshCompiler( "SkeletalMeshCompiler", s_version )
    {
        m_outputTypes.push_back( SkeletalMesh::GetStaticResourceTypeID() );
    }

    Resource::CompilationResult SkeletalMeshCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        SkeletalMeshResourceDescriptor resourceDescriptor;
        if ( !Resource::ResourceDescriptor::TryReadFromFile( *m_pTypeRegistry, ctx.m_inputFilePath, resourceDescriptor ) )
        {
            return Error( "Failed to read resource descriptor from input file: %s", ctx.m_inputFilePath.c_str() );
        }

        // Read mesh data
        //-------------------------------------------------------------------------

        FileSystem::Path meshFilePath;
        if ( !ConvertResourcePathToFilePath( resourceDescriptor.m_meshPath, meshFilePath ) )
        {
            return Error( "Invalid mesh data path: %s", resourceDescriptor.m_meshPath.c_str() );
        }

        RawAssets::ReaderContext readerCtx = { [this]( char const* pString ) { Warning( pString ); }, [this] ( char const* pString ) { Error( pString ); } };
        int32_t const maxBoneInfluences = 4;
        TUniquePtr<RawAssets::RawMesh> pRawMesh = RawAssets::ReadSkeletalMesh( readerCtx, meshFilePath, resourceDescriptor.m_meshesToInclude, maxBoneInfluences );
        if ( pRawMesh == nullptr )
        {
            return Error( "Failed to read mesh from source file" );
        }

        EE_ASSERT( pRawMesh->IsValid() );

        if ( resourceDescriptor.m_mergeSectionsByMaterial )
        {
            pRawMesh->MergeGeometrySectionsByMaterial();
        }

        // Reflect FBX data into runtime format
        //-------------------------------------------------------------------------

        SkeletalMesh skeletalMesh;
        TransferMeshGeometry( *pRawMesh, skeletalMesh, maxBoneInfluences );
        OptimizeMeshGeometry( skeletalMesh );
        TransferSkeletalMeshData( *pRawMesh, skeletalMesh );
        SetMeshDefaultMaterials( resourceDescriptor, skeletalMesh );

        // Serialize
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( s_version, SkeletalMesh::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );
        SetMeshInstallDependencies( skeletalMesh, hdr );

        Serialization::BinaryOutputArchive archive;
        archive << hdr << skeletalMesh;

        if ( archive.WriteToFile( ctx.m_outputFilePath ) )
        {
            if ( pRawMesh->HasWarnings() )
            {
                return CompilationSucceededWithWarnings( ctx );
            }
            else
            {
                return CompilationSucceeded( ctx );
            }
        }
        else
        {
            return CompilationFailed( ctx );
        }
    }

    void SkeletalMeshCompiler::TransferSkeletalMeshData( RawAssets::RawMesh const& rawMesh, SkeletalMesh& mesh ) const
    {
        EE_ASSERT( rawMesh.IsSkeletalMesh() );

        auto const& skeleton = rawMesh.GetSkeleton();
        auto const& boneData = skeleton.GetBoneData();

        auto const numBones = skeleton.GetNumBones();
        for ( auto i = 0u; i < numBones; i++ )
        {
            mesh.m_boneIDs.push_back( boneData[i].m_name );
            mesh.m_parentBoneIndices.push_back( boneData[i].m_parentBoneIdx );
            mesh.m_bindPose.push_back( boneData[i].m_globalTransform );
            mesh.m_inverseBindPose.push_back( boneData[i].m_globalTransform.GetInverse() );
        }
    }
}