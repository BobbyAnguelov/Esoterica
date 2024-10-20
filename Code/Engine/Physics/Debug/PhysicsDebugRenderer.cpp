#include "PhysicsDebugRenderer.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Physics/Systems/WorldSystem_Physics.h"
#include "Engine/Entity/EntityWorld.h"
#include "Base/Profiling.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Physics
{
    using namespace EE::Render;

    //-------------------------------------------------------------------------

    bool PhysicsRenderer::Initialize( Render::RenderDevice* pRenderDevice )
    {
        EE_ASSERT( m_pRenderDevice == nullptr && pRenderDevice != nullptr );
        m_pRenderDevice = pRenderDevice;

        //-------------------------------------------------------------------------

        if ( !m_pointRS.Initialize( m_pRenderDevice ) )
        {
            return false;
        }

        if ( !m_lineRS.Initialize( m_pRenderDevice ) )
        {
            return false;
        }

        if ( !m_primitiveRS.Initialize( m_pRenderDevice ) )
        {
            return false;
        }

        if ( !m_textRS.Initialize( m_pRenderDevice ) )
        {
            return false;
        }

        m_initialized = true;
        return m_initialized;
    }

    void PhysicsRenderer::Shutdown()
    {
        if ( m_pRenderDevice != nullptr )
        {
            m_textRS.Shutdown( m_pRenderDevice );
            m_primitiveRS.Shutdown( m_pRenderDevice );
            m_lineRS.Shutdown( m_pRenderDevice );
            m_pointRS.Shutdown( m_pRenderDevice );
        }

        m_pRenderDevice = nullptr;
        m_initialized = false;
    }

    //-------------------------------------------------------------------------

    void PhysicsRenderer::DrawPoints( RenderContext const& renderContext, Render::Viewport const& viewport, physx::PxDebugPoint const* pPoints, uint32_t numPoints )
    {
        m_pointRS.SetState( renderContext, viewport );
        renderContext.SetDepthTestMode( DepthTestMode::Off );
        renderContext.SetPrimitiveTopology( Topology::PointList );

        auto pData = reinterpret_cast<Drawing::PointCommand*>( m_pointRS.m_stagingVertexData.data() );
        auto const dataSize = (uint32_t) m_pointRS.m_stagingVertexData.size();

        //-------------------------------------------------------------------------

        uint32_t currentPointIdx = 0;
        uint32_t const numDrawCalls = (uint32_t) Math::Ceiling( (float) numPoints / DebugPointRenderState::MaxPointsPerDrawCall );
        for ( auto i = 0u; i < numDrawCalls; i++ )
        {
            uint32_t bufferVertexIdx = 0;
            uint32_t const numPointsToDraw = Math::Min( numPoints - currentPointIdx, DebugPointRenderState::MaxPointsPerDrawCall );
            for ( auto p = 0u; p < numPointsToDraw; p++ )
            {
                physx::PxDebugPoint const& point = pPoints[currentPointIdx + p];
                pData[bufferVertexIdx].m_position = Float3( point.pos.x, point.pos.y, point.pos.z );
                pData[bufferVertexIdx].m_thickness = 1.0f;
                pData[bufferVertexIdx].m_color = Physics::FromPxColor( point.color );
                bufferVertexIdx++;
            }

            currentPointIdx += numPointsToDraw;
            renderContext.WriteToBuffer( m_pointRS.m_vertexBuffer, pData, dataSize );
            renderContext.Draw( numPointsToDraw, 0 );
        }
    }

    void PhysicsRenderer::DrawLines( RenderContext const& renderContext, Render::Viewport const& viewport, physx::PxDebugLine const* pLines, uint32_t numLines )
    {
        m_lineRS.SetState( renderContext, viewport );
        renderContext.SetDepthTestMode( DepthTestMode::Off );
        renderContext.SetPrimitiveTopology( Topology::LineList );

        auto pData = reinterpret_cast<Drawing::LineCommand*>( m_lineRS.m_stagingVertexData.data() );
        auto const dataSize = (uint32_t) m_lineRS.m_stagingVertexData.size();

        //-------------------------------------------------------------------------

        uint32_t currentLineIdx = 0;
        uint32_t const numDrawCalls = (uint32_t) Math::Ceiling( (float) numLines / DebugLineRenderState::MaxLinesPerDrawCall );
        for ( auto i = 0u; i < numDrawCalls; i++ )
        {
            uint32_t bufferVertexIdx = 0;
            uint32_t const numLinesToDraw = Math::Min( numLines - currentLineIdx, DebugLineRenderState::MaxLinesPerDrawCall );
            for ( auto l = 0u; l < numLinesToDraw; l++ )
            {
                physx::PxDebugLine const& line = pLines[currentLineIdx + l];
                pData[bufferVertexIdx].m_startPosition = Float3( line.pos0.x, line.pos0.y, line.pos0.z );
                pData[bufferVertexIdx].m_startThickness = 1.0f;
                pData[bufferVertexIdx].m_startColor = Physics::FromPxColor( line.color0 );
                pData[bufferVertexIdx].m_endPosition = Float3( line.pos1.x, line.pos1.y, line.pos1.z );
                pData[bufferVertexIdx].m_endThickness = 1.0f;
                pData[bufferVertexIdx].m_endColor = Physics::FromPxColor( line.color1 );
                bufferVertexIdx++;
            }

            currentLineIdx += numLinesToDraw;
            renderContext.WriteToBuffer( m_lineRS.m_vertexBuffer, pData, dataSize );
            renderContext.Draw( numLinesToDraw * 2, 0 );
        }
    }

    void PhysicsRenderer::DrawTriangles( RenderContext const& renderContext, Render::Viewport const& viewport, physx::PxDebugTriangle const* pTriangles, uint32_t numTriangles )
    {
        m_primitiveRS.SetState( renderContext, viewport );
        renderContext.SetDepthTestMode( DepthTestMode::Off );
        renderContext.SetPrimitiveTopology( Topology::TriangleList );

        auto pData = reinterpret_cast<Drawing::TriangleCommand*>( m_primitiveRS.m_stagingVertexData.data() );
        auto const dataSize = (uint32_t) m_primitiveRS.m_stagingVertexData.size();

        //-------------------------------------------------------------------------

        uint32_t currentTriangleIdx = 0;
        uint32_t const numDrawCalls = (uint32_t) Math::Ceiling( (float) numTriangles / DebugPrimitiveRenderState::MaxTrianglesPerDrawCall );
        for ( auto i = 0u; i < numDrawCalls; i++ )
        {
            uint32_t bufferVertexIdx = 0;
            uint32_t const numTrianglesToDraw = Math::Min( numTriangles - currentTriangleIdx, DebugPrimitiveRenderState::MaxTrianglesPerDrawCall );
            for ( auto t = 0u; t < numTrianglesToDraw; t++ )
            {
                physx::PxDebugTriangle const& tri = pTriangles[currentTriangleIdx + t];
                pData[bufferVertexIdx].m_vertex0 = Float4( tri.pos0.x, tri.pos0.y, tri.pos0.z, 1.0f );
                pData[bufferVertexIdx].m_color0 = Physics::FromPxColor( tri.color0 );
                pData[bufferVertexIdx].m_vertex1 = Float4( tri.pos1.x, tri.pos1.y, tri.pos1.z, 1.0f );
                pData[bufferVertexIdx].m_color1 = Physics::FromPxColor( tri.color1 );
                pData[bufferVertexIdx].m_vertex2 = Float4( tri.pos2.x, tri.pos2.y, tri.pos2.z, 1.0f );
                pData[bufferVertexIdx].m_color2 = Physics::FromPxColor( tri.color2 );
                bufferVertexIdx++;
            }

            currentTriangleIdx += numTrianglesToDraw;
            renderContext.WriteToBuffer( m_primitiveRS.m_vertexBuffer, pData, dataSize );
            renderContext.Draw( numTrianglesToDraw * 3, 0 );
        }
    }

    void PhysicsRenderer::DrawStrings( RenderContext const& renderContext, Render::Viewport const& viewport, physx::PxDebugText const* pStrings, uint32_t numStrings )
    {
        m_textRS.SetState( renderContext, viewport );
        renderContext.SetDepthTestMode( DepthTestMode::Off );

        //-------------------------------------------------------------------------

        auto pVertexData = EE_STACK_ARRAY_ALLOC( DebugFontGlyphVertex, DebugTextRenderState::MaxGlyphsPerDrawCall * 4 );
        auto const vertexDataSize = sizeof( DebugFontGlyphVertex ) * DebugTextRenderState::MaxGlyphsPerDrawCall * 4;

        auto pIndexData = EE_STACK_ARRAY_ALLOC( uint16_t, DebugTextRenderState::MaxGlyphsPerDrawCall * 6 );
        auto const indexDataSize = sizeof( uint16_t ) * DebugTextRenderState::MaxGlyphsPerDrawCall * 6;

        int32_t numGlyphsDrawn = 0;
        for ( auto c = 0u; c < numStrings; c++ )
        {
            physx::PxDebugText const& debugText = pStrings[c];

            // Get the glyph string and number of glyphs needed to render it
            //-------------------------------------------------------------------------

            TInlineVector<int32_t, 100> glyphIndices;
            m_textRS.m_fontAtlas.GetGlyphsForString( 0, debugText.string, glyphIndices );

            int32_t const glyphCount = (int32_t) glyphIndices.size();
            int32_t const remainingGlyphsPerDrawCall = ( DebugTextRenderState::MaxGlyphsPerDrawCall - numGlyphsDrawn - glyphCount );

            // If we are going to overflow the buffer, draw all previously recorded glyphs
            if ( remainingGlyphsPerDrawCall < 0 )
            {
                renderContext.WriteToBuffer( m_textRS.m_vertexBuffer, pVertexData, vertexDataSize );
                renderContext.WriteToBuffer( m_textRS.m_indexBuffer, pIndexData, indexDataSize );
                renderContext.DrawIndexed( numGlyphsDrawn * 6, 0 );
                numGlyphsDrawn = 0;
            }

            EE_ASSERT( ( DebugTextRenderState::MaxGlyphsPerDrawCall - numGlyphsDrawn - glyphCount ) >= 0 );

            // Draw string
            //-------------------------------------------------------------------------

            Float2 const textPosTopLeft = viewport.WorldSpaceToScreenSpace( Physics::FromPx( debugText.position ) ) + 0.5f;
            Float4 const color = Physics::FromPxColor( debugText.color );
            numGlyphsDrawn += m_textRS.m_fontAtlas.WriteGlyphsToBuffer( &pVertexData[numGlyphsDrawn * 4], uint16_t( numGlyphsDrawn * 4 ), &pIndexData[numGlyphsDrawn * 6], 0, glyphIndices, textPosTopLeft, color );
        }

        // Draw glyphs
        if ( numGlyphsDrawn > 0 )
        {
            renderContext.WriteToBuffer( m_textRS.m_vertexBuffer, pVertexData, vertexDataSize );
            renderContext.WriteToBuffer( m_textRS.m_indexBuffer, pIndexData, indexDataSize );
            renderContext.DrawIndexed( numGlyphsDrawn * 6, 0 );
        }
    }

    //-------------------------------------------------------------------------

    void PhysicsRenderer::RenderWorld( Seconds const deltaTime, Render::Viewport const& viewport, Render::RenderTarget const& renderTarget, EntityWorld* pWorld )
    {
        EE_ASSERT( WasInitialized() && Threading::IsMainThread() );
        EE_PROFILE_FUNCTION_RENDER();

        //-------------------------------------------------------------------------

        auto pPhysicsWorldSystem = pWorld->GetWorldSystem<PhysicsWorldSystem>();
        auto pPhysicsWorld = pPhysicsWorldSystem->GetWorld();
        if ( !pPhysicsWorld->IsDebugDrawingEnabled() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        if ( !viewport.IsValid() )
        {
            return;
        }

        auto const& renderContext = m_pRenderDevice->GetImmediateContext();
        renderContext.SetRenderTarget( renderTarget );
        renderContext.SetViewport( Float2( viewport.GetDimensions() ), Float2( viewport.GetTopLeftPosition() ) );

        //-------------------------------------------------------------------------

        // Offset the culling bounds in front of the camera, no point in visualizing lines off-screen
        float const debugHalfDistance = ( pPhysicsWorld->GetDebugDrawDistance() );

        Vector const viewForward = viewport.GetViewForwardDirection();
        Vector cullingBoundsPosition = viewport.GetViewPosition();
        cullingBoundsPosition += viewForward * debugHalfDistance;

        AABB const debugBounds = AABB( cullingBoundsPosition, debugHalfDistance );
        pPhysicsWorld->SetDebugCullingBox( debugBounds );

        //-------------------------------------------------------------------------

        physx::PxRenderBuffer const& renderBuffer = pPhysicsWorld->GetRenderBuffer();

        uint32_t const numPoints = renderBuffer.getNbPoints();
        DrawPoints( renderContext, viewport, renderBuffer.getPoints(), numPoints );

        uint32_t const numLines = renderBuffer.getNbLines();
        DrawLines( renderContext, viewport, renderBuffer.getLines(), numLines );

        uint32_t const numTriangles = renderBuffer.getNbTriangles();
        DrawTriangles( renderContext, viewport, renderBuffer.getTriangles(), numTriangles );
    }
}
#endif