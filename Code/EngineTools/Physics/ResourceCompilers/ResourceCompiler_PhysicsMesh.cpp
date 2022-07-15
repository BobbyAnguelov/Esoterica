#include "ResourceCompiler_PhysicsMesh.h"
#include "Engine/Physics/PhysX.h"
#include "EngineTools/Physics/ResourceDescriptors/ResourceDescriptor_PhysicsMesh.h"
#include "EngineTools/RawAssets/RawAssetReader.h"
#include "EngineTools/RawAssets/RawMesh.h"
#include "Engine/Physics/PhysicsMesh.h"
#include "Engine/Physics/PhysicsMaterial.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Serialization/BinarySerialization.h"


//-------------------------------------------------------------------------

using namespace physx;

//-------------------------------------------------------------------------

namespace EE
{
    namespace Physics
    {
        class PhysxMemoryStream : public PxOutputStream
        {
        public:

            PhysxMemoryStream( Blob& destination ) : m_buffer( destination ) {}

        private:

            virtual PxU32 write( const void* src, PxU32 count ) override final
            {
                size_t originalSize = m_buffer.size();
                m_buffer.resize( originalSize + count );
                memcpy( &m_buffer[originalSize], src, count );
                return count;
            }

        private:

            Blob& m_buffer;
        };

        //-------------------------------------------------------------------------

        PhysicsMeshCompiler::PhysicsMeshCompiler()
            : Resource::Compiler( "PhysicsMeshCompiler", s_version )
        {
            m_outputTypes.push_back( PhysicsMesh::GetStaticResourceTypeID() );
        }

        Resource::CompilationResult PhysicsMeshCompiler::Compile( Resource::CompileContext const& ctx ) const
        {
            PhysicsMeshResourceDescriptor resourceDescriptor;
            if ( !Resource::ResourceDescriptor::TryReadFromFile( *m_pTypeRegistry, ctx.m_inputFilePath, resourceDescriptor ) )
            {
                return Error( "Failed to read resource descriptor from input file: %s", ctx.m_inputFilePath.c_str() );
            }

            // Read mesh data
            //-------------------------------------------------------------------------

            FileSystem::Path meshFilePath;
            if ( !ConvertResourcePathToFilePath( resourceDescriptor.m_meshPath, meshFilePath ) )
            {
                return Error( "Invalid source data path: %s", resourceDescriptor.m_meshPath.c_str() );
            }

            RawAssets::ReaderContext readerCtx = { [this]( char const* pString ) { Warning( pString ); }, [this] ( char const* pString ) { Error( pString ); } };
            TUniquePtr<RawAssets::RawMesh> pRawMesh = RawAssets::ReadStaticMesh( readerCtx, meshFilePath, resourceDescriptor.m_meshName );
            if ( pRawMesh == nullptr )
            {
                return Error( "Failed to read mesh from source file" );
            }

            EE_ASSERT( pRawMesh->IsValid() );

            // Reflect FBX data into physics format
            //-------------------------------------------------------------------------
            
            PhysicsMesh physicsMesh;
            Blob cookedMeshData;

            if ( resourceDescriptor.m_isConvexMesh )
            {
                if ( !CookConvexMeshData( *pRawMesh, cookedMeshData ) )
                {
                    return CompilationFailed( ctx );
                }

                physicsMesh.m_isConvexMesh = true;
            }
            else
            {
                if ( !CookTriangleMeshData( *pRawMesh, cookedMeshData ) )
                {
                    return CompilationFailed( ctx );
                }

                physicsMesh.m_isConvexMesh = false;
            }

            // Set Materials
            //-------------------------------------------------------------------------
            // For now just use the default material until we have a proper DCC physics pipeline

            static StringID const defaultMaterialID( PhysicsMaterial::DefaultID );
            if ( physicsMesh.IsConvexMesh() )
            {
                physicsMesh.m_physicsMaterialIDs.emplace_back( defaultMaterialID );
            }
            else // One material per geometry section
            {
                for ( auto i = 0; i < pRawMesh->GetNumGeometrySections(); i++ )
                {
                    physicsMesh.m_physicsMaterialIDs.emplace_back( defaultMaterialID );
                }
            }

            // Serialize
            //-------------------------------------------------------------------------

            Resource::ResourceHeader hdr( s_version, PhysicsMesh::GetStaticResourceTypeID() );
            Serialization::BinaryOutputArchive archive;
            archive << hdr << physicsMesh << cookedMeshData;

            if ( archive.WriteToFile( ctx.m_outputFilePath ) )
            {
                if( pRawMesh->HasWarnings() )
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

        bool PhysicsMeshCompiler::CookTriangleMeshData( RawAssets::RawMesh const& rawMesh, Blob& outCookedData ) const
        {
            PhysXAllocator allocator;
            PhysXUserErrorCallback errorCallback;

            PxTolerancesScale tolerancesScale;
            tolerancesScale.length = Constants::s_lengthScale;
            tolerancesScale.speed = Constants::s_speedScale;

            auto pFoundation = PxCreateFoundation( PX_PHYSICS_VERSION, allocator, errorCallback );
            auto pCooking = PxCreateCooking( PX_PHYSICS_VERSION, *pFoundation, PxCookingParams( tolerancesScale ) );
            if ( pCooking == nullptr)
            {
                pFoundation->release();
                pFoundation = nullptr;
                Error( "PxCreateCooking failed!" );
                return false;
            }

            // Reflect raw mesh into physx
            //-------------------------------------------------------------------------
            // PhysX meshes require counterclockwise winding

            PxTriangleMeshDesc meshDesc;
            meshDesc.points.stride = sizeof( float ) * 3;
            meshDesc.triangles.stride = sizeof( uint32_t ) * 3;
            meshDesc.materialIndices.stride = sizeof( PxMaterialTableIndex );

            TVector<Float3> vertexData;
            TVector<uint32_t> indexData;
            TVector<PxMaterialTableIndex> materialIndexData;

            PxMaterialTableIndex materialIdx = 0;
            for ( auto const& geometrySection : rawMesh.GetGeometrySections() )
            {
                // Add the verts
                for ( auto const& vert : geometrySection.m_vertices )
                {
                    vertexData.push_back( vert.m_position );
                }

                // Add the indices - taking into account offset from previously added verts
                for ( auto idx : geometrySection.m_indices )
                {
                    indexData.push_back( meshDesc.points.count + idx );
                }

                // Add material indices
                uint32_t const numTriangles = geometrySection.GetNumTriangles();
                for ( auto triIdx = 0u; triIdx < geometrySection.GetNumTriangles(); triIdx++ )
                {
                    materialIndexData.emplace_back( materialIdx );
                }

                meshDesc.points.count += (uint32_t) geometrySection.m_vertices.size();
                meshDesc.triangles.count += numTriangles;
                materialIdx++;
            }

            meshDesc.points.data = vertexData.data();
            meshDesc.triangles.data = indexData.data();
            meshDesc.materialIndices.data = materialIndexData.data();

            //-------------------------------------------------------------------------

            outCookedData.clear();
            PhysxMemoryStream stream( outCookedData );
            PxTriangleMeshCookingResult::Enum result;
            pCooking->cookTriangleMesh( meshDesc, stream, &result );

            //-------------------------------------------------------------------------

            pCooking->release();
            pFoundation->release();
            pFoundation = nullptr;

            //-------------------------------------------------------------------------

            if ( result == PxTriangleMeshCookingResult::eLARGE_TRIANGLE )
            {
                Error( "Triangle mesh cooking failed - Large triangle detected" );
                return false;
            }
            else if ( result == PxTriangleMeshCookingResult::eFAILURE )
            {
                Error( "Triangle mesh cooking failed!" );
                return false;
            }

            return true;
        }

        bool PhysicsMeshCompiler::CookConvexMeshData( RawAssets::RawMesh const& rawMesh, Blob& outCookedData ) const
        {
            PhysXAllocator allocator;
            PhysXUserErrorCallback errorCallback;

            PxTolerancesScale tolerancesScale;
            tolerancesScale.length = Constants::s_lengthScale;
            tolerancesScale.speed = Constants::s_speedScale;

            auto pFoundation = PxCreateFoundation( PX_PHYSICS_VERSION, allocator, errorCallback );
            auto pCooking = PxCreateCooking( PX_PHYSICS_VERSION, *pFoundation, PxCookingParams( tolerancesScale ) );
            if ( pCooking == nullptr )
            {
                pFoundation->release();
                pFoundation = nullptr;
                Error( "PxCreateCooking failed!" );
                return false;
            }

            // Reflect raw mesh into a physx convex mesh
            //-------------------------------------------------------------------------

            TVector<Float3> vertexData;
            TVector<uint32_t> indexData;
            uint32_t indexOffset = 0;

            for ( auto const& geometrySection : rawMesh.GetGeometrySections() )
            {
                // Add the verts
                for ( auto const& vert : geometrySection.m_vertices )
                {
                    vertexData.push_back( vert.m_position );
                }

                // Add the indices - taking into account offset from previously added verts
                for ( auto idx : geometrySection.m_indices )
                {
                    indexData.push_back( indexOffset + idx );
                }

                indexOffset += (uint32_t) geometrySection.m_vertices.size();
            }

            //-------------------------------------------------------------------------

            PxConvexMeshDesc convexMeshDesc;
            convexMeshDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;
            convexMeshDesc.points.count = (uint32_t) vertexData.size();
            convexMeshDesc.points.stride = sizeof( float ) * 3;
            convexMeshDesc.points.data = vertexData.data();

            convexMeshDesc.indices.count = (uint32_t) indexData.size();
            convexMeshDesc.indices.stride = sizeof( uint32_t ) * 3;
            convexMeshDesc.indices.data = indexData.data();

            //-------------------------------------------------------------------------

            outCookedData.clear();
            PhysxMemoryStream stream( outCookedData );

            PxConvexMeshCookingResult::Enum result;
            pCooking->cookConvexMesh( convexMeshDesc, stream, &result );

            //-------------------------------------------------------------------------

            pCooking->release();
            pFoundation->release();
            pFoundation = nullptr;

            //-------------------------------------------------------------------------

            if ( result == PxConvexMeshCookingResult::eZERO_AREA_TEST_FAILED )
            {
                Error( "Convex mesh cooking failed - Zero area test failed" );
                return false;
            }
            else if ( result == PxConvexMeshCookingResult::ePOLYGONS_LIMIT_REACHED )
            {
                Error( "Convex mesh cooking failed - Polygon limit reached" );
                return false;
            }
            else if ( result == PxTriangleMeshCookingResult::eFAILURE )
            {
                Error( "Convex mesh cooking failed!" );
                return false;
            }

            return true;
        }
    }
}
