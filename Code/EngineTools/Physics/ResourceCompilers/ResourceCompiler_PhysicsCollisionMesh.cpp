#include "ResourceCompiler_PhysicsCollisionMesh.h"
#include "EngineTools/Physics/ResourceDescriptors/ResourceDescriptor_PhysicsCollisionMesh.h"
#include "EngineTools/Resource/ResourceCompilerContext.h"
#include "EngineTools/Import/Importer.h"
#include "EngineTools/Import/ImportedMesh.h"
#include "Engine/Physics/PhysicsCollisionMesh.h"
#include "Engine/Physics/Physics.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    CollisionMeshCompiler::CollisionMeshCompiler()
        : Resource::Compiler( "CollisionMeshCompiler" )
    {
        RegisterOutput<CollisionMesh>();
    }

    uint64_t CollisionMeshCompiler::GetAdditionalVersionForResourceType( ResourceTypeID resourceTypeID ) const
    {
        return B3_MESH_VERSION;
    }

    Resource::CompilationResult CollisionMeshCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        auto pResourceDescriptor = ctx.GetDescriptor<PhysicsCollisionMeshResourceDescriptor>();
        if ( Math::IsNearZero( pResourceDescriptor->m_scale.m_x ) || Math::IsNearZero( pResourceDescriptor->m_scale.m_y ) || Math::IsNearZero( pResourceDescriptor->m_scale.m_z ) )
        {
            return ctx.LogError( "Zero Scale is not allowed!" );
        }

        // Create collision mesh
        //-------------------------------------------------------------------------

        CollisionMesh physicsMesh;
        physicsMesh.m_collisionSettings = pResourceDescriptor->m_collisionSettings;

        // Read raw mesh data
        //-------------------------------------------------------------------------

        FileSystem::Path const meshFilePath = pResourceDescriptor->m_sourcePath.GetFileSystemPath( ctx.m_sourceResourceDirectoryPath );

        Import::ReaderContext readerCtx = { [&ctx]( char const* pString ) { ctx.LogWarning( pString ); }, [&ctx] ( char const* pString ) { ctx.LogError( pString ); } };
        Import::Source const fileSource( meshFilePath, ctx.GetRawData( pResourceDescriptor->m_sourcePath ) );
        TUniquePtr<Import::Mesh> pImportedMesh = Import::Importer::ReadStaticMesh( readerCtx, fileSource, pResourceDescriptor->m_meshesToInclude );
        if ( pImportedMesh == nullptr )
        {
            return ctx.LogError( "Failed to read mesh from source file" );
        }

        EE_ASSERT( pImportedMesh->IsValid() );
        pImportedMesh->ApplyScale( pResourceDescriptor->m_scale );

        auto const& meshGeometries = pImportedMesh->GetGeometries();

        // Reflect mesh data into physics format
        //-------------------------------------------------------------------------

        int32_t materialIdx = 0;

        if ( pResourceDescriptor->m_isConvexHull )
        {
            TVector<Float3> points;

            for ( auto const& submesh : pImportedMesh->GetSubmeshes() )
            {
                auto const& geo = meshGeometries[submesh.m_geometryIdx];

                for ( auto const& vert : geo.m_vertices )
                {
                    points.push_back( submesh.m_transform.TransformPoint( vert.m_position ) );
                }

                materialIdx++;
            }

            b3HullData* pHull = b3CreateHull( (b3Vec3*) points.data(), (int32_t) points.size(), (int32_t) points.size() );
            physicsMesh.m_bounds = FromBox3D( pHull->aabb );
            physicsMesh.m_isConvexHull = true;

            physicsMesh.m_shapeData.resize( pHull->byteCount );
            memcpy( physicsMesh.m_shapeData.data(), pHull, pHull->byteCount );

            b3DestroyHull( pHull );
        }
        else
        {
            TVector<Float3> vertices;
            TVector<int32_t> indices;
            TVector<uint8_t> materialIndices;
            uint32_t indexOffset = 0;

            for ( auto const& submesh : pImportedMesh->GetSubmeshes() )
            {
                auto const& geo = meshGeometries[submesh.m_geometryIdx];

                // Add the vertices
                for ( auto const& vert : geo.m_vertices )
                {
                    vertices.push_back( submesh.m_transform.TransformPoint( vert.m_position ) );
                }

                // Add the indices - taking into account offset from previously added verts
                for ( auto idx : geo.m_indices )
                {
                    indices.push_back( indexOffset + idx );
                }

                // Add material indices
                uint32_t const numTriangles = geo.GetNumTriangles();
                for ( auto triIdx = 0u; triIdx < geo.GetNumTriangles(); triIdx++ )
                {
                    EE_ASSERT( materialIdx < 255 );
                    materialIndices.emplace_back( (uint8_t) materialIdx );
                }

                indexOffset += (uint32_t) geo.m_vertices.size();
                materialIdx++;
            }

            b3MeshDef def = {};
            def.vertices = (b3Vec3*) vertices.data();
            def.vertexCount = (int32_t) vertices.size();
            def.indices = indices.data();
            def.triangleCount = (int32_t) indices.size() / 3;
            def.materialIndices = materialIndices.data();
            def.useMedianSplit = false;
            def.identifyEdges = true;
            def.weldVertices = true;
            def.weldTolerance = 0.002f;

            b3MeshData* pMesh = b3CreateMesh( &def, nullptr, 0 );
            physicsMesh.m_bounds = FromBox3D( pMesh->bounds );
            physicsMesh.m_isConvexHull = false;

            physicsMesh.m_shapeData.resize( pMesh->byteCount );
            memcpy( physicsMesh.m_shapeData.data(), pMesh, pMesh->byteCount );

            b3DestroyMesh( pMesh );
        }

        // Set Materials
        //-------------------------------------------------------------------------
        // For now just use the default material until we have a proper DCC physics pipeline

        for ( auto i = 0; i < materialIdx; i++ )
        {
            physicsMesh.m_materialIDs.emplace_back( Material::s_defaultID );
        }

        // Serialize
        //-------------------------------------------------------------------------

        Resource::ResourceHeader hdr( CollisionMesh::s_version, CollisionMesh::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );
        Serialization::BinaryOutputArchive archive;
        archive << hdr << physicsMesh;

        if ( archive.WriteToFile( ctx.GetOutputPath() ) )
        {
            if ( pImportedMesh->HasWarnings() )
            {
                return Resource::CompilationResult::SuccessWithWarnings;
            }
            else
            {
                return Resource::CompilationResult::Success;
            }
        }
        else
        {
            return Resource::CompilationResult::Failure;
        }
    }
}
