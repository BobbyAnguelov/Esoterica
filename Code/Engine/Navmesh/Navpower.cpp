#if EE_ENABLE_NAVPOWER
#include "NavPower.h"
#include "System/Drawing/DebugDrawingSystem.h"

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    #if EE_DEVELOPMENT_TOOLS
    void NavpowerRenderer::DrawLineList( bfx::LineSegment const* pLines, uint32_t numLines, bfx::Color const& color )
    {
        auto ctx = m_pDebugDrawingSystem->GetDrawingContext();
        for ( auto i = 0u; i < numLines; i++ )
        {
            bfx::LineSegment const& line = pLines[i];
            ctx.DrawLine( FromBfx( line.m_v0 ), FromBfx( line.m_v1 ), FromBfx( color ), 1.0f, m_depthTestEnabled ? Drawing::EnableDepthTest : Drawing::DisableDepthTest );
        }
    }

    void NavpowerRenderer::DrawTriList( bfx::Triangle const* pTris, uint32_t numTris, bfx::Color const& color )
    {
        auto ctx = m_pDebugDrawingSystem->GetDrawingContext();
        for ( auto i = 0u; i < numTris; i++ )
        {
            bfx::Triangle const& tri = pTris[i];
            ctx.DrawTriangle( FromBfx( tri.m_v0 ), FromBfx( tri.m_v1 ), FromBfx( tri.m_v2 ), FromBfx( color ), m_depthTestEnabled ? Drawing::EnableDepthTest : Drawing::DisableDepthTest );
        }
    }

    void NavpowerRenderer::DrawString( bfx::Color const& color, char const* str )
    {
        auto ctx = m_pDebugDrawingSystem->GetDrawingContext();
        ctx.DrawText2D( m_statsPos, str, FromBfx( color ), Drawing::FontSmall );
        m_statsPos += Float2( 0, 15 );
    }

    void NavpowerRenderer::DrawString( bfx::Color const& color, bfx::Vector3 const& pos, char const* str )
    {
        auto ctx = m_pDebugDrawingSystem->GetDrawingContext();
        ctx.DrawText3D( FromBfx( pos ), str, FromBfx( color ), Drawing::FontSmall );
    }
    #endif
}
#endif