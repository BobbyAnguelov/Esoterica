#pragma once
#if EE_ENABLE_NAVPOWER
#include "System/Math/Transform.h"
#include "System/Types/Color.h"
#include "System/Memory/Memory.h"

#include <bfxSystemSpace.h>
#include <bfxPlannerSpace.h>

//-------------------------------------------------------------------------

namespace bfx { class CustomAllocator; }
namespace EE::Drawing { class DrawingSystem; }

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    //-------------------------------------------------------------------------
    // Type Conversion
    //-------------------------------------------------------------------------

    EE_FORCE_INLINE Float3 FromBfx( bfx::Vector3 const& v )
    {
        return Float3( v.m_x, v.m_y, v.m_z );
    }

    EE_FORCE_INLINE Quaternion FromBfx( bfx::Quaternion const& v )
    {
        return Quaternion( v.m_x, v.m_y, v.m_z, v.m_w );
    }

    EE_FORCE_INLINE Color FromBfx( bfx::Color const& color )
    {
        return Color( uint8_t( color.m_r * 255 ), uint8_t( color.m_g * 255 ), uint8_t( color.m_b ), uint8_t( color.m_a * 255 ) );
    }

    //-------------------------------------------------------------------------

    EE_FORCE_INLINE bfx::Vector3 ToBfx( Float3 const& v )
    {
        return bfx::Vector3( v.m_x, v.m_y, v.m_z );
    }

    EE_FORCE_INLINE bfx::Vector3 ToBfx( Float4 const& v )
    {
        return bfx::Vector3( v.m_x, v.m_y, v.m_z );
    }

    EE_FORCE_INLINE bfx::Vector3 ToBfx( Vector const& v )
    {
        Float3 const f3 = v.ToFloat3();
        return ToBfx( f3 );
    }

    EE_FORCE_INLINE bfx::Quaternion ToBfx( Quaternion const& q )
    {
        Float4 const f4 = q.ToFloat4();

        bfx::Quaternion quaternion;
        quaternion.m_x = f4.m_x;
        quaternion.m_y = f4.m_y;
        quaternion.m_z = f4.m_z;
        quaternion.m_w = f4.m_w;
        return quaternion;
    }

    EE_FORCE_INLINE bfx::Color ToBfx( Color const& color )
    {
        return bfx::Color( color.m_byteColor.m_r / 255.0f, color.m_byteColor.m_g / 255.0f, color.m_byteColor.m_b / 255.0f, color.m_byteColor.m_a / 255.0f );
    }

    //-------------------------------------------------------------------------
    // System and Allocator
    //-------------------------------------------------------------------------

    namespace NavPower
    {
        void Initialize();
        void Shutdown();
        bfx::CustomAllocator* GetAllocator();
    }
}
#endif