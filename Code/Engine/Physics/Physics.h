#pragma once
#include "Engine/_Module/API.h"
#include "Base/Math/Transform.h"
#include "Base/Math/Plane.h"
#include "Base/Math/BoundingVolumes.h"
#include "Base/Types/Color.h"
#include "Base/Memory/Memory.h"

//-------------------------------------------------------------------------

#if _MSC_VER
#pragma warning( push, 0 )
#pragma warning( disable : 4435; disable : 4996 )
#endif

#include <PxPhysicsAPI.h>
#include <extensions/PxDefaultAllocator.h>
#include <extensions/PxDefaultErrorCallback.h>

#if _MSC_VER
#pragma warning( pop )
#endif

//-------------------------------------------------------------------------
// WARNING!
// Do not include this in any header with a reflected type!
// Ideally try to always include this in CPP files!
//-------------------------------------------------------------------------

namespace EE
{
    class UpdateContext;
}

//-------------------------------------------------------------------------

namespace EE::Physics
{
    //-------------------------------------------------------------------------
    // Shared State: Constants/Conversion/Shapes
    //-------------------------------------------------------------------------

    struct EE_ENGINE_API Constants
    {
        static constexpr float const    s_lengthScale = 1.0f;
        static constexpr float const    s_speedScale = 9.81f;
        static Float3 const             s_gravity;
    };

    namespace PX
    {
        struct Conversion
        {
            static Quaternion const         s_capsuleConversionToPx;
            static Quaternion const         s_capsuleConversionFromPx;
        };

        struct Shapes
        {
            static physx::PxConvexMesh*     s_pUnitCylinderMesh;
        };

        class Allocator final : public physx::PxAllocatorCallback
        {
            virtual void* allocate( size_t size, const char* typeName, const char* filename, int line ) override
            {
                return EE::Alloc( size, 16 );
            }

            virtual void deallocate( void* ptr ) override
            {
                EE::Free( ptr );
            }
        };

        class UserErrorCallback final : public physx::PxErrorCallback
        {
            virtual void reportError( physx::PxErrorCode::Enum code, const char* message, const char* file, int line ) override
            {
                if ( code >= physx::PxErrorCode::eINVALID_PARAMETER )
                {
                    EE_HALT();
                }

                Log::AddEntry( Log::Severity::Error, "Physics", "PhysX Error", file, line, message );
            }
        };
    }

    //-------------------------------------------------------------------------
    // Physics State
    //-------------------------------------------------------------------------

    class MaterialDatabase;

    namespace Core
    {
        void Initialize();
        void Shutdown();

        physx::PxPhysics* GetPxPhysics();
    };

    //-------------------------------------------------------------------------
    // Type Conversion
    //-------------------------------------------------------------------------

    EE_FORCE_INLINE Float2 FromPx( physx::PxVec2 const& v )
    {
        return Float2( v.x, v.y );
    }

    EE_FORCE_INLINE Float3 FromPx( physx::PxVec3 const& v )
    {
        return Float3( v.x, v.y, v.z );
    }

    EE_FORCE_INLINE Float3 FromPx( physx::PxExtendedVec3 const& v )
    {
        return Float3( (float) v.x, (float) v.y, (float) v.z );
    }

    EE_FORCE_INLINE Float4 FromPx( physx::PxVec4 const& v )
    {
        return Float4( v.x, v.y, v.z, v.w );
    }

    EE_FORCE_INLINE Quaternion FromPx( physx::PxQuat const& v )
    {
        return Quaternion( v.x, v.y, v.z, v.w );
    }

    EE_FORCE_INLINE Transform FromPx( physx::PxTransform const& v )
    {
        return Transform( FromPx( v.q ), FromPx( v.p ) );
    }

    EE_FORCE_INLINE Plane FromPx( physx::PxPlane const& v )
    {
        return Plane::FromNormalAndDistance( FromPx( v.n ), v.d );
    }

    EE_FORCE_INLINE Matrix FromPx( physx::PxMat33 const& v )
    {
        Matrix const m
        (
            v.column0[0], v.column1[0], v.column2[0], 0.0f,
            v.column0[1], v.column1[1], v.column2[1], 0.0f,
            v.column0[2], v.column1[2], v.column2[2], 0.0f,
            0, 0, 0, 1.0f
        );

        return m;
    }

    EE_FORCE_INLINE Matrix FromPx( physx::PxMat44 const& v )
    {
        return (Matrix&) v;
    }

    EE_FORCE_INLINE AABB FromPx( physx::PxBounds3 const& bounds )
    {
        return AABB::FromMinMax( FromPx( bounds.minimum ), FromPx( bounds.maximum ) );
    }

    EE_FORCE_INLINE Color FromPxColor( physx::PxU32 color )
    {
        return Color( color | 0x000000FF );
    }

    // Convert a PhysX X-height capsule to an Esoterica Z-height capsule
    EE_FORCE_INLINE Transform FromPxCapsuleTransform( physx::PxTransform const& v )
    {
        Transform const transform( PX::Conversion::s_capsuleConversionFromPx * FromPx( v.q ), FromPx( v.p ) );
        return transform;
    }

    // Is this a game physics scene
    EE_FORCE_INLINE bool IsGameScene( physx::PxScene const* pScene ) { return pScene->userData != nullptr; }

    // Is this a tool physics scene
    EE_FORCE_INLINE bool IsToolsScene( physx::PxScene const* pScene ) { return pScene->userData != nullptr; }

    //-------------------------------------------------------------------------

    EE_FORCE_INLINE physx::PxVec2 ToPx( Float2 const& v )
    {
        return physx::PxVec2( v.m_x, v.m_y );
    }

    EE_FORCE_INLINE physx::PxVec3 ToPx( Float3 const& v )
    {
        return physx::PxVec3( v.m_x, v.m_y, v.m_z );
    }

    EE_FORCE_INLINE physx::PxExtendedVec3 ToPxExtended( Float3 const& v )
    {
        return physx::PxExtendedVec3( v.m_x, v.m_y, v.m_z );
    }

    EE_FORCE_INLINE physx::PxVec3 ToPx( Vector const& v )
    {
        Float3 const f3 = v.ToFloat3();
        return physx::PxVec3( f3.m_x, f3.m_y, f3.m_z );
    }

    EE_FORCE_INLINE physx::PxExtendedVec3 ToPxExtended( Vector const& v )
    {
        Float3 const f3 = v.ToFloat3();
        return physx::PxExtendedVec3( f3.m_x, f3.m_y, f3.m_z );
    }

    EE_FORCE_INLINE physx::PxVec4 ToPx( Float4 const& v )
    {
        return physx::PxVec4( v.m_x, v.m_y, v.m_z, v.m_w );
    }

    EE_FORCE_INLINE physx::PxQuat ToPx( Quaternion const& v )
    {
        Float4 const f4 = v.ToFloat4();
        return physx::PxQuat( f4.m_x, f4.m_y, f4.m_z, f4.m_w );
    }

    EE_FORCE_INLINE physx::PxTransform ToPx( Transform const& v )
    {
        EE_ASSERT( v.IsRigidTransform() );
        return physx::PxTransform( ToPx( v.GetTranslation().ToFloat3() ), ToPx( v.GetRotation() ) );
    }

    EE_FORCE_INLINE physx::PxPlane ToPx( Plane const& v )
    {
        EE_ASSERT( v.IsNormalized() );
        return physx::PxPlane( v.a, v.b, v.c, v.d );
    }

    EE_FORCE_INLINE physx::PxMat33 ToPxMat33( Matrix const& v )
    {
        physx::PxMat33 m( physx::PxIdentity );

        m.column0[0] = v[0][0];
        m.column0[1] = v[1][0];
        m.column0[2] = v[2][0];

        m.column1[0] = v[0][1];
        m.column1[1] = v[1][1];
        m.column1[2] = v[2][1];

        m.column2[0] = v[0][2];
        m.column2[1] = v[1][2];
        m.column2[2] = v[2][2];

        return m;
    }

    EE_FORCE_INLINE physx::PxMat44 ToPxMat44( Matrix const& v )
    {
        return (physx::PxMat44&) v;
    }

    EE_FORCE_INLINE physx::PxBounds3 ToPx( AABB const& bounds )
    {
        return physx::PxBounds3( ToPx( bounds.GetMin() ), ToPx( bounds.GetMax() ) );
    }

    // Convert an esoterica Z-height capsule to a physX X-height capsule
    EE_FORCE_INLINE physx::PxTransform ToPxCapsuleTransform( Transform const& v )
    {
        physx::PxTransform const transform( ToPx( v.GetTranslation().ToFloat3() ), ToPx( PX::Conversion::s_capsuleConversionToPx * v.GetRotation() ) );
        return transform;
    }

    //-------------------------------------------------------------------------

    class EE_BASE_API [[nodiscard]] ScopedWriteLock
    {
    public:

        EE_FORCE_INLINE ScopedWriteLock( physx::PxScene* pScene ) : m_pScene( pScene ) { pScene->lockWrite(); }
        EE_FORCE_INLINE ~ScopedWriteLock() { m_pScene->unlockWrite(); }

    private:

        physx::PxScene* m_pScene = nullptr;
    };

    //-------------------------------------------------------------------------

    class EE_BASE_API [[nodiscard]] ScopedReadLock
    {
    public:

        EE_FORCE_INLINE ScopedReadLock( physx::PxScene* pScene ) : m_pScene( pScene ) { pScene->lockRead(); }
        EE_FORCE_INLINE ~ScopedReadLock() { m_pScene->unlockRead(); }

    private:

        physx::PxScene* m_pScene = nullptr;
    };
}