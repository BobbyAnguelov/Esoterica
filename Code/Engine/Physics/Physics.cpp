#include "Physics.h"
#include "Engine/Render/DebugMesh/DebugMeshRegistry.h"
#include "Base/Math/MathUtils.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Memory::Allocators
{
    static MemoryAllocator g_box3D( "Box 3D" );
}

//-------------------------------------------------------------------------
namespace EE::Physics
{
    //-------------------------------------------------------------------------
    // Helpers
    //-------------------------------------------------------------------------

    static TVector<Float4>* g_pUnitCylinderHullVertices = nullptr;

    TVector<Float4> const& GetUnitCylinderHullPoints()
    {
        EE_ASSERT( g_pUnitCylinderHullVertices != nullptr );
        return *g_pUnitCylinderHullVertices;
    }

    //-------------------------------------------------------------------------
    // Debug
    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void CreateDebugMeshFromTriangleMesh( b3MeshData const* pMesh, Render::DebugMesh& outMesh )
    {
        b3Vec3 const* pVerts = b3GetMeshVertices( pMesh );
        b3MeshTriangle const* pTris = b3GetMeshTriangles( pMesh );

        for ( int32_t t = 0; t < pMesh->triangleCount; t++ )
        {
            auto const& tri = pTris[t];

            Vector const v0 = Vector( FromBox3D( pVerts[tri.index1] ) );
            Vector const v1 = Vector( FromBox3D( pVerts[tri.index2] ) );
            Vector const v2 = Vector( FromBox3D( pVerts[tri.index3] ) );

            Vector const s0 = ( v1 - v0 ).GetNormalized3();
            Vector const s1 = ( v2 - v1 ).GetNormalized3();
            Vector const normal = s0.Cross3( s1 ).GetNormalized3();

            outMesh.m_vertices.emplace_back( v0, normal );
            outMesh.m_vertices.emplace_back( v1, normal );
            outMesh.m_vertices.emplace_back( v2, normal );
        }
    }

    void CreateDebugMeshFromConvexHull( b3HullData const* pHull, Render::DebugMesh& outMesh )
    {
        b3Vec3 const* pHullPoints = b3GetHullPoints( pHull );
        b3HullHalfEdge const* pHullEdges = b3GetHullEdges( pHull );
        b3HullFace const* pHullFaces = b3GetHullFaces( pHull );

        for ( int i = 0; i < pHull->faceCount; ++i )
        {
            const b3HullFace* face = pHullFaces + i;

            const b3HullHalfEdge* pEdge1 = pHullEdges + face->edge;
            const b3HullHalfEdge* pEdge2 = pHullEdges + pEdge1->next;
            const b3HullHalfEdge* pEdge3 = pHullEdges + pEdge2->next;
            B3_ASSERT( pEdge1 != pEdge3 );

            do
            {
                Vector const v0 = Vector( FromBox3D( pHullPoints[pEdge1->origin] ) );
                Vector const v1 = Vector( FromBox3D( pHullPoints[pEdge2->origin] ) );
                Vector const v2 = Vector( FromBox3D( pHullPoints[pEdge3->origin] ) );

                Vector const s0 = ( v1 - v0 ).GetNormalized3();
                Vector const s1 = ( v2 - v1 ).GetNormalized3();
                Vector const normal = s0.Cross3( s1 ).GetNormalized3();

                outMesh.m_vertices.emplace_back( v0, normal );
                outMesh.m_vertices.emplace_back( v1, normal );
                outMesh.m_vertices.emplace_back( v2, normal );

                pEdge2 = pEdge3;
                pEdge3 = pHullEdges + pEdge3->next;

            } while ( pEdge1 != pEdge3 );
        }
    }

    void CreateHeightFieldMesh( b3HeightFieldData const* pHeightfield, Render::DebugMesh& outMesh )
    {
        EE_UNIMPLEMENTED_FUNCTION();
    }

    //-------------------------------------------------------------------------

    struct DebugShapeInstance
    {
        b3ShapeType             m_type = b3_capsuleShape;
        uint64_t                m_sourceDataID = 0; // The ID we used to track the instances of the source data
        uint64_t                m_debugMeshID = UINT64_MAX;
        Vector                  m_scale = Vector::One;
        Vector                  m_center0;
        Vector                  m_center1;
        float                   m_radius;
    };

    struct DebugShapeRegistry
    {
        struct RefCountShape
        {
            uint64_t            m_debugMeshID = UINT64_MAX;
            int32_t             m_refCount = 0;
        };

    public:

        ~DebugShapeRegistry()
        {
            EE_ASSERT( m_registeredShapes.empty() );
        }

        PointerID RegisterShape( Render::DebugMeshRegistry* pDebugMeshRegistry, b3DebugShape const* pDebugShape )
        {
            EE_ASSERT( pDebugShape != nullptr );

            auto pNewInstance = m_registeredShapes.emplace_back( EE::New<DebugShapeInstance>() );
            pNewInstance->m_type = pDebugShape->type;

            //-------------------------------------------------------------------------

            switch ( pDebugShape->type )
            {
                case b3_hullShape:
                {
                    pNewInstance->m_sourceDataID = Hash::GetHash64( pDebugShape->hull, pDebugShape->hull->byteCount );

                    // Get debug mesh ID
                    auto iter = m_debugShapes.find( pNewInstance->m_sourceDataID );
                    if ( iter == m_debugShapes.end() )
                    {
                        Render::DebugMesh hullMesh;
                        CreateDebugMeshFromConvexHull( pDebugShape->hull, hullMesh );
                        pNewInstance->m_debugMeshID = pDebugMeshRegistry->RegisterMesh( "Physics Hull", hullMesh );

                        RefCountShape rcs;
                        rcs.m_debugMeshID = pNewInstance->m_debugMeshID;
                        rcs.m_refCount = 1;
                        m_debugShapes[pNewInstance->m_sourceDataID] = rcs;
                    }
                    else
                    {
                        iter->second.m_refCount++;
                        pNewInstance->m_debugMeshID = iter->second.m_debugMeshID;
                    }
                }
                break;

                case b3_meshShape:
                {
                    pNewInstance->m_sourceDataID = (uintptr_t) pDebugShape->mesh->data;
                    pNewInstance->m_scale = FromBox3D( pDebugShape->mesh->scale );

                    // Get debug mesh ID
                    auto iter = m_debugShapes.find( pNewInstance->m_sourceDataID );
                    if ( iter == m_debugShapes.end() )
                    {
                        Render::DebugMesh mesh;
                        CreateDebugMeshFromTriangleMesh( pDebugShape->mesh->data, mesh );
                        pNewInstance->m_debugMeshID = pDebugMeshRegistry->RegisterMesh( "Physics Mesh", mesh );

                        RefCountShape rcs;
                        rcs.m_debugMeshID = pNewInstance->m_debugMeshID;
                        rcs.m_refCount = 1;
                        m_debugShapes[pNewInstance->m_sourceDataID] = rcs;
                    }
                    else
                    {
                        iter->second.m_refCount++;
                        pNewInstance->m_debugMeshID = iter->second.m_debugMeshID;
                    }
                }
                break;

                case b3_heightShape:
                {
                    pNewInstance->m_sourceDataID = (uintptr_t) pDebugShape->heightField;

                    // Get debug mesh ID
                    auto iter = m_debugShapes.find( pNewInstance->m_sourceDataID );
                    if ( iter == m_debugShapes.end() )
                    {
                        Render::DebugMesh heightfieldMesh;
                        CreateHeightFieldMesh( pDebugShape->heightField, heightfieldMesh );
                        pNewInstance->m_debugMeshID = pDebugMeshRegistry->RegisterMesh( "Physics Heightfield", heightfieldMesh );

                        RefCountShape rcs;
                        rcs.m_debugMeshID = pNewInstance->m_debugMeshID;
                        rcs.m_refCount = 1;
                        m_debugShapes[pNewInstance->m_sourceDataID] = rcs;
                    }
                    else
                    {
                        iter->second.m_refCount++;
                        pNewInstance->m_debugMeshID = iter->second.m_debugMeshID;
                    }
                }
                break;

                case b3_sphereShape:
                {
                    pNewInstance->m_center0 = FromBox3D( pDebugShape->sphere->center );
                    pNewInstance->m_center1 = FromBox3D( pDebugShape->sphere->center );
                    pNewInstance->m_radius = pDebugShape->sphere->radius;
                }
                break;

                case b3_capsuleShape:
                {
                    pNewInstance->m_center0 = FromBox3D( pDebugShape->capsule->center1 );
                    pNewInstance->m_center1 = FromBox3D( pDebugShape->capsule->center2 );
                    pNewInstance->m_radius = pDebugShape->capsule->radius;
                }
                break;

                default:
                break;
            }

            return PointerID( pNewInstance );
        }

        void UnregisterShape( Render::DebugMeshRegistry* pDebugMeshRegistry, PointerID instanceID )
        {
            auto pInstance = reinterpret_cast<DebugShapeInstance*>( instanceID.ToVoidPtr() );

            for ( size_t i = 0; i < m_registeredShapes.size(); i++ )
            {
                if ( pInstance == m_registeredShapes[i] )
                {
                    switch ( m_registeredShapes[i]->m_type )
                    {
                        case b3_hullShape:
                        case b3_meshShape:
                        case b3_heightShape:
                        {
                            auto iter = m_debugShapes.find( m_registeredShapes[i]->m_sourceDataID );
                            EE_ASSERT( iter != m_debugShapes.end() );
                            iter->second.m_refCount--;
                            if ( iter->second.m_refCount == 0 )
                            {
                                pDebugMeshRegistry->UnregisterMesh( iter->second.m_debugMeshID );
                                m_debugShapes.erase( iter );
                            }
                        }
                        break;

                        case b3_sphereShape:
                        case b3_capsuleShape:
                        default:
                        break;
                    }

                    //-------------------------------------------------------------------------

                    EE::Delete( m_registeredShapes[i] );
                    m_registeredShapes.erase( m_registeredShapes.begin() + i );
                    return;
                }
            }

            EE_UNREACHABLE_CODE();
        }

    public:

        THashMap<uint64_t, RefCountShape>               m_debugShapes;
        TVector<DebugShapeInstance*>                    m_registeredShapes;
    };
    #endif

    //-------------------------------------------------------------------------
    // Core
    //-------------------------------------------------------------------------

    namespace Core
    {
        #if EE_DEVELOPMENT_TOOLS
        static DebugShapeRegistry *g_pShapeRegistry = nullptr;
        #endif

        //-------------------------------------------------------------------------

        static void* Alloc( int32_t size, int32_t alignment )
        {
            return Memory::Allocators::g_box3D.Alloc( size, alignment );
        }

        static void Free( void* pMemory )
        {
            Memory::Allocators::g_box3D.Free( pMemory );
        }

        static int32_t Assert( const char* condition, const char* fileName, int lineNumber )
        {
            SystemLog::LogAssert( fileName, lineNumber, condition );
            return 1;
        }

        void Initialize()
        {
            b3SetAllocator( &Alloc, &Free );
            b3SetAssertFcn( &Assert );

            //-------------------------------------------------------------------------

            g_pUnitCylinderHullVertices = EE::New<TVector<Float4>>();
            g_pUnitCylinderHullVertices->resize( 32 );

            Math::CreateCircleVertices( Vector::UnitX, Vector::UnitZ, 16, g_pUnitCylinderHullVertices->data() );

            TVector<Float4>& verts = *g_pUnitCylinderHullVertices;
            for ( int32_t i = 0; i < 16; i++ )
            {
                verts[i + 16] = verts[i] - Float4( 0, 0, 1.0f, 0.0f );
                verts[i] += Float4( 0, 0, 1.0f, 0.0f );
            }

            #if EE_DEVELOPMENT_TOOLS
            g_pShapeRegistry = EE::New<DebugShapeRegistry>();
            #endif
        }

        void Shutdown()
        {
            #if EE_DEVELOPMENT_TOOLS
            EE::Delete( g_pShapeRegistry );
            #endif

            EE::Delete( g_pUnitCylinderHullVertices );
        }

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        PointerID RegisterDebugShapeInstance( Render::DebugMeshRegistry* pDebugMeshRegistry, b3DebugShape const* pDebugShape )
        {
            EE_ASSERT( g_pShapeRegistry != nullptr );
            return g_pShapeRegistry->RegisterShape( pDebugMeshRegistry, pDebugShape );
        }

        void UnregisterDebugShapeInstance( Render::DebugMeshRegistry* pDebugMeshRegistry, PointerID ID )
        {
            EE_ASSERT( ID.IsValid() );
            EE_ASSERT( g_pShapeRegistry != nullptr );
            g_pShapeRegistry->UnregisterShape( pDebugMeshRegistry, ID );
        }

        void DrawDebugShapeInstance( DebugDrawContext* pDrawContext, PointerID instanceID, Transform const& transform, Color const& color )
        {
            EE_ASSERT( pDrawContext != nullptr );
            EE_ASSERT( instanceID.IsValid() );

            auto pInstance = reinterpret_cast<DebugShapeInstance*>( instanceID.ToVoidPtr() );

            switch ( pInstance->m_type )
            {
                case b3_capsuleShape:
                {
                    Vector const c1 = transform.TransformPoint( pInstance->m_center0 );
                    Vector const c2 = transform.TransformPoint( pInstance->m_center1 );
                    pDrawContext->DrawLitCapsule( c1, c2, pInstance->m_radius, color );
                }
                break;

                //-------------------------------------------------------------------------

                case b3_sphereShape:
                {
                    Vector center = transform.TransformPoint( pInstance->m_center0 );
                    pDrawContext->DrawLitSphere( center, pInstance->m_radius, color );
                }
                break;

                //-------------------------------------------------------------------------

                case b3_hullShape:
                case b3_meshShape:
                {
                    pDrawContext->DrawLitMesh( pInstance->m_debugMeshID, transform.GetTranslation(), transform.GetRotation(), pInstance->m_scale, color );
                }
                break;

                case b3_heightShape:
                {
                    pDrawContext->DrawLitMesh( pInstance->m_debugMeshID, transform, color );
                }
                break;

                default:
                break;
            }
        }
        #endif
    }
}