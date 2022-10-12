#include "DebugRenderer.h"
#include "System/Profiling.h"
#include "System/Types/Function.h"
#include "Engine/Entity/EntityWorld.h"

//-------------------------------------------------------------------------

using namespace EE::Drawing;

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Render
{
    bool DebugRenderer::Initialize( RenderDevice* pRenderDevice )
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

        //-------------------------------------------------------------------------

        m_intermediateGlyphVertexData.resize( DebugTextRenderState::MaxGlyphsPerDrawCall * 4 );
        m_intermediateGlyphIndexData.resize( DebugTextRenderState::MaxGlyphsPerDrawCall * 6 );

        //-------------------------------------------------------------------------

        m_initialized = true;
        return m_initialized;
    }

    void DebugRenderer::Shutdown()
    {
        m_drawCommands.Clear();

        //-------------------------------------------------------------------------

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

    void DebugRenderer::DrawPoints( RenderContext const& renderContext, Viewport const& viewport, TVector<PointCommand> const& commands )
    {
        // Set render state
        renderContext.SetPrimitiveTopology( Topology::PointList );

        //-------------------------------------------------------------------------

        auto pData = reinterpret_cast<Float4*>( m_pointRS.m_stagingVertexData.data() );
        auto const dataSize = (uint32_t) m_pointRS.m_stagingVertexData.size();

        int32_t const numPoints = (int32_t) commands.size();
        if ( numPoints > 0 )
        {
            // Fill the points vertex buffer
            uint32_t const numDrawCalls = (uint32_t) Math::Ceiling( (float) numPoints / DebugPointRenderState::MaxPointsPerDrawCall );
            for ( auto i = 0u; i < numDrawCalls; i++ )
            {
                uint32_t const drawRangeStart = ( i * DebugPointRenderState::MaxPointsPerDrawCall );
                uint32_t const drawRangeEnd = drawRangeStart + Math::Min( uint32_t( numPoints - ( i * DebugPointRenderState::MaxPointsPerDrawCall ) ), (uint32_t) DebugPointRenderState::MaxPointsPerDrawCall );
                uint32_t const drawRangeLength = ( drawRangeEnd - drawRangeStart );
                uint32_t const drawCommandMemorySize = drawRangeLength * sizeof( PointCommand );
                memcpy( pData, &commands[drawRangeStart], drawCommandMemorySize );

                renderContext.WriteToBuffer( m_pointRS.m_vertexBuffer, pData, dataSize );
                renderContext.Draw( drawRangeLength, 0 );
            }
        }
    }

    void DebugRenderer::DrawLines( RenderContext const& renderContext, Viewport const& viewport, TVector<LineCommand> const& commands )
    {
        // Set render state
        renderContext.SetPrimitiveTopology( Topology::LineList );

        //-------------------------------------------------------------------------

        auto pData = reinterpret_cast<Float4*>( m_lineRS.m_stagingVertexData.data() );
        auto const dataSize = (uint32_t) m_lineRS.m_stagingVertexData.size();

        int32_t const numLines = (int32_t) commands.size();
        if ( numLines > 0 )
        {
            // Fill the lines vertex buffer
            uint32_t const numDrawCalls = (uint32_t) Math::Ceiling( (float) numLines / DebugLineRenderState::MaxLinesPerDrawCall );
            for ( auto i = 0u; i < numDrawCalls; i++ )
            {
                uint32_t const drawRangeStart = ( i * DebugLineRenderState::MaxLinesPerDrawCall );
                uint32_t const drawRangeEnd = drawRangeStart + Math::Min( uint32_t( numLines - ( i * DebugLineRenderState::MaxLinesPerDrawCall ) ), (uint32_t) DebugLineRenderState::MaxLinesPerDrawCall );
                uint32_t const drawRangeLength = ( drawRangeEnd - drawRangeStart );
                uint32_t const drawCommandMemorySize = drawRangeLength * sizeof( LineCommand );
                memcpy( pData, &commands[drawRangeStart], drawCommandMemorySize );

                renderContext.WriteToBuffer( m_lineRS.m_vertexBuffer, pData, dataSize );
                renderContext.Draw( drawRangeLength * 2, 0 );
            }
        }
    }

    void DebugRenderer::DrawTriangles( RenderContext const& renderContext, Viewport const& viewport, TVector<TriangleCommand> const& commands )
    {
        // Set render state
        renderContext.SetPrimitiveTopology( Topology::TriangleList );

        //-------------------------------------------------------------------------

        auto pData = reinterpret_cast<Float4*>( m_primitiveRS.m_stagingVertexData.data() );
        auto const dataSize = (uint32_t) m_primitiveRS.m_stagingVertexData.size();

        int32_t const numTriangles = (int32_t) commands.size();
        if ( numTriangles > 0 )
        {
            // Fill the triangles vertex buffer
            uint32_t const numDrawCalls = (uint32_t) Math::Ceiling( (float) numTriangles / DebugPrimitiveRenderState::MaxTrianglesPerDrawCall );
            for ( auto i = 0u; i < numDrawCalls; i++ )
            {
                uint32_t const drawRangeStart = ( i * DebugPrimitiveRenderState::MaxTrianglesPerDrawCall );
                uint32_t const drawRangeEnd = drawRangeStart + Math::Min( numTriangles - ( i * DebugPrimitiveRenderState::MaxTrianglesPerDrawCall ), DebugPrimitiveRenderState::MaxTrianglesPerDrawCall );
                uint32_t const drawRangeLength = ( drawRangeEnd - drawRangeStart );
                uint32_t const drawCommandMemorySize = drawRangeLength * sizeof( TriangleCommand );
                memcpy( pData, &commands[drawRangeStart], drawCommandMemorySize );

                renderContext.WriteToBuffer( m_primitiveRS.m_vertexBuffer, pData, dataSize );
                renderContext.Draw( drawRangeLength * 3, 0 );
            }
        }
    }

    void DebugRenderer::DrawText( RenderContext const& renderContext, Viewport const& viewport, TVector<TextCommand> const& commands, IntRange cmdRange )
    {
        EE_ASSERT( cmdRange.IsValid() );

        int32_t const fontIdx = ( commands[cmdRange.m_begin].m_fontSize == FontSmall ) ? 0 : 1;

        //-------------------------------------------------------------------------

        auto pVertexData = m_intermediateGlyphVertexData.data();
        size_t const vertexDataSize = m_intermediateGlyphVertexData.size() * sizeof( DebugFontGlyphVertex );

        auto pIndexData = m_intermediateGlyphIndexData.data();
        size_t const indexDataSize = m_intermediateGlyphIndexData.size() * sizeof( uint16_t );

        //-------------------------------------------------------------------------

        int32_t numGlyphsDrawn = 0;
        for ( auto c = cmdRange.m_begin; c < cmdRange.m_end; c++ )
        {
            auto const& cmd = commands[c];

            // Get the glyph string and number of glyphs needed to render it
            //-------------------------------------------------------------------------

            TInlineVector<int32_t, 100> glyphIndices;
            m_textRS.m_fontAtlas.GetGlyphsForString( fontIdx, cmd.m_text, glyphIndices );

            int32_t numGlyphsToDraw = (int32_t) glyphIndices.size();
            if ( cmd.m_hasBackground )
            {
                numGlyphsToDraw++;
            }

            // If we are going to overflow the buffer, draw all previously recorded glyphs
            if ( DebugTextRenderState::MaxGlyphsPerDrawCall < numGlyphsDrawn + numGlyphsToDraw )
            {
                renderContext.WriteToBuffer( m_textRS.m_vertexBuffer, pVertexData, vertexDataSize );
                renderContext.WriteToBuffer( m_textRS.m_indexBuffer, pIndexData, indexDataSize );
                renderContext.DrawIndexed( numGlyphsDrawn * 6, 0 );
                numGlyphsDrawn = 0;
            }

            EE_ASSERT( DebugTextRenderState::MaxGlyphsPerDrawCall >= numGlyphsDrawn + numGlyphsToDraw );

            // Get text top left position
            //-------------------------------------------------------------------------

            Float2 textPosTopLeft;
            if (  cmd.m_isScreenText )
            {
                textPosTopLeft = (Float2) cmd.m_position;
            }
            else // World Space
            {
                // Ensure that the world position is on-screen
                Vector const textPosTopLeftCS = viewport.WorldSpaceToClipSpace( cmd.m_position );
                if ( textPosTopLeftCS.GetW() < 0 )
                {
                    continue;
                }

                textPosTopLeft = viewport.ClipSpaceToScreenSpace( textPosTopLeftCS );
            }

            // Adjust text position based on alignment settings
            //-------------------------------------------------------------------------

            static int32_t const textBoxPadding = 3;
            Float2 const extents( m_textRS.m_fontAtlas.GetTextExtents( fontIdx, cmd.m_text.c_str() ) );

            switch ( cmd.m_alignment )
            {
                case AlignTopLeft:
                {
                    if ( cmd.m_hasBackground )
                    {
                        textPosTopLeft.m_x += textBoxPadding;
                        textPosTopLeft.m_y += textBoxPadding;
                    }
                }
                break;

                case AlignTopCenter:
                {
                    textPosTopLeft.m_x -= ( extents.m_x / 2 );

                    if ( cmd.m_hasBackground )
                    {
                        textPosTopLeft.m_x -= 0.5f * textBoxPadding;
                        textPosTopLeft.m_y += textBoxPadding;
                    }
                }
                break;

                case AlignTopRight:
                {
                    textPosTopLeft.m_x -= extents.m_x;

                    if ( cmd.m_hasBackground )
                    {
                        textPosTopLeft.m_x -= textBoxPadding;
                        textPosTopLeft.m_y += textBoxPadding;
                    }
                }
                break;

                case AlignMiddleLeft:
                {
                    textPosTopLeft.m_y -= extents.m_y / 2;

                    if ( cmd.m_hasBackground )
                    {
                        textPosTopLeft.m_x += textBoxPadding;
                        textPosTopLeft.m_y -= 0.5f * textBoxPadding;
                    }
                }
                break;

                case AlignMiddleCenter:
                {
                    textPosTopLeft.m_x -= ( extents.m_x / 2 );
                    textPosTopLeft.m_y -= extents.m_y / 2;

                    if ( cmd.m_hasBackground )
                    {
                        textPosTopLeft.m_x -= 0.5f * textBoxPadding;
                        textPosTopLeft.m_y -= 0.5f * textBoxPadding;
                    }
                }
                break;

                case AlignMiddleRight:
                {
                    textPosTopLeft.m_x -= extents.m_x;
                    textPosTopLeft.m_y -= extents.m_y / 2;

                    if ( cmd.m_hasBackground )
                    {
                        textPosTopLeft.m_x -= textBoxPadding;
                        textPosTopLeft.m_y -= 0.5f * textBoxPadding;
                    }
                }
                break;

                case AlignBottomLeft:
                {
                    textPosTopLeft.m_y -= extents.m_y;

                    if ( cmd.m_hasBackground )
                    {
                        textPosTopLeft.m_x += textBoxPadding;
                        textPosTopLeft.m_y -= textBoxPadding;
                    }
                }
                break;

                case AlignBottomCenter:
                {
                    textPosTopLeft.m_x -= ( extents.m_x / 2 );
                    textPosTopLeft.m_y -= extents.m_y;

                    if ( cmd.m_hasBackground )
                    {
                        textPosTopLeft.m_x -= 0.5f * textBoxPadding;
                        textPosTopLeft.m_y -= textBoxPadding;
                    }
                }
                break;

                case AlignBottomRight:
                {
                    textPosTopLeft.m_x -= extents.m_x;
                    textPosTopLeft.m_y -= extents.m_y;

                    if ( cmd.m_hasBackground )
                    {
                        textPosTopLeft.m_x -= textBoxPadding;
                        textPosTopLeft.m_y -= textBoxPadding;
                    }
                }
                break;
            }

            // Draw characters
            //-------------------------------------------------------------------------

            EE_ASSERT( numGlyphsDrawn < DebugTextRenderState::MaxGlyphsPerDrawCall );

            if ( cmd.m_hasBackground )
            {
                textPosTopLeft.m_y += textBoxPadding;
                m_textRS.m_fontAtlas.WriteCustomGlyphToBuffer( &pVertexData[numGlyphsDrawn * 4], uint16_t( numGlyphsDrawn * 4 ), &pIndexData[numGlyphsDrawn * 6], fontIdx, glyphIndices[0], m_textRS.m_nonZeroAlphaTexCoords, textPosTopLeft, extents, textBoxPadding, Float4( 0, 0, 0, 128.0f / 255 ) );
                numGlyphsDrawn++;
            }

            numGlyphsDrawn += m_textRS.m_fontAtlas.WriteGlyphsToBuffer( &pVertexData[numGlyphsDrawn * 4], uint16_t( numGlyphsDrawn * 4 ), &pIndexData[numGlyphsDrawn * 6], fontIdx, glyphIndices, textPosTopLeft, cmd.m_color );
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

    static void DrawTextCommands( TVector<TextCommand> const& commands, RenderContext const& renderContext, Viewport const& viewport, TFunction<void( RenderContext const&, Viewport const&, TVector<TextCommand> const&, IntRange )> renderFunction )
    {
        uint32_t const numCommands = (uint32_t) commands.size();
        IntRange cmdRange = IntRange( 0, 0 );
        for ( uint32_t i = cmdRange.m_begin; i < numCommands; i++ )
        {
            // If the text sizes differ
            if ( commands[i].m_fontSize != commands[cmdRange.m_begin].m_fontSize )
            {
                cmdRange.m_end = i;
                renderFunction( renderContext, viewport, commands, cmdRange );
                cmdRange.m_begin = i;
            }

            if ( i == ( numCommands - 1 ) )
            {
                cmdRange.m_end = numCommands;
                renderFunction( renderContext, viewport, commands, cmdRange );
            }
        }
    }

    //-------------------------------------------------------------------------

    void DebugRenderer::RenderWorld( Seconds const deltaTime, Viewport const& viewport, RenderTarget const& renderTarget, EntityWorld* pWorld )
    {
        EE_ASSERT( IsInitialized() && Threading::IsMainThread() );
        EE_PROFILE_FUNCTION_RENDER();

        if ( !viewport.IsValid() )
        {
            return;
        }

        auto pDebugDrawingSystem = pWorld->GetDebugDrawingSystem();
        EE_ASSERT( pDebugDrawingSystem != nullptr );
        pDebugDrawingSystem->ReflectFrameCommandBuffer( deltaTime, m_drawCommands );

        //-------------------------------------------------------------------------

        auto const& renderContext = m_pRenderDevice->GetImmediateContext();
        renderContext.SetRenderTarget( renderTarget );
        renderContext.SetViewport( Float2( viewport.GetDimensions() ), Float2( viewport.GetTopLeftPosition() ) );

        //-------------------------------------------------------------------------
        // Draw Points
        //-------------------------------------------------------------------------

        {
            EE_PROFILE_SCOPE_RENDER( "DebugDrawing::DrawPoints" );
            m_pointRS.SetState( renderContext, viewport );

            renderContext.SetDepthTestMode( DepthTestMode::On );
            DebugRenderer::DrawPoints( renderContext, viewport, m_drawCommands.m_opaqueDepthOn.m_pointCommands );

            renderContext.SetDepthTestMode( DepthTestMode::Off );
            DebugRenderer::DrawPoints( renderContext, viewport, m_drawCommands.m_opaqueDepthOff.m_pointCommands );

            //-------------------------------------------------------------------------

            renderContext.SetDepthTestMode( DepthTestMode::On );
            DebugRenderer::DrawPoints( renderContext, viewport, m_drawCommands.m_transparentDepthOn.m_pointCommands );

            renderContext.SetDepthTestMode( DepthTestMode::Off );
            DebugRenderer::DrawPoints( renderContext, viewport, m_drawCommands.m_transparentDepthOff.m_pointCommands );
        }

        //-------------------------------------------------------------------------
        // Draw Lines
        //-------------------------------------------------------------------------

        {
            EE_PROFILE_SCOPE_RENDER( "DebugDrawing::DrawLines" );
            m_lineRS.SetState( renderContext, viewport );

            renderContext.SetDepthTestMode( DepthTestMode::On );
            DebugRenderer::DrawLines( renderContext, viewport, m_drawCommands.m_opaqueDepthOn.m_lineCommands );

            renderContext.SetDepthTestMode( DepthTestMode::Off );
            DebugRenderer::DrawLines( renderContext, viewport, m_drawCommands.m_opaqueDepthOff.m_lineCommands );

            //-------------------------------------------------------------------------

            renderContext.SetDepthTestMode( DepthTestMode::On );
            DebugRenderer::DrawLines( renderContext, viewport, m_drawCommands.m_transparentDepthOn.m_lineCommands );

            renderContext.SetDepthTestMode( DepthTestMode::Off );
            DebugRenderer::DrawLines( renderContext, viewport, m_drawCommands.m_transparentDepthOff.m_lineCommands );
        }

        //-------------------------------------------------------------------------
        // Draw Primitives
        //-------------------------------------------------------------------------

        {
            EE_PROFILE_SCOPE_RENDER( "DebugDrawing::DrawPrimitives" );
            m_primitiveRS.SetState( renderContext, viewport );

            renderContext.SetDepthTestMode( DepthTestMode::On );
            DebugRenderer::DrawTriangles( renderContext, viewport, m_drawCommands.m_opaqueDepthOn.m_triangleCommands );

            renderContext.SetDepthTestMode( DepthTestMode::Off );
            DebugRenderer::DrawTriangles( renderContext, viewport, m_drawCommands.m_opaqueDepthOff.m_triangleCommands );

            //-------------------------------------------------------------------------

            renderContext.SetDepthTestMode( DepthTestMode::On );
            DebugRenderer::DrawTriangles( renderContext, viewport, m_drawCommands.m_transparentDepthOn.m_triangleCommands );

            renderContext.SetDepthTestMode( DepthTestMode::Off );
            DebugRenderer::DrawTriangles( renderContext, viewport, m_drawCommands.m_transparentDepthOff.m_triangleCommands );
        }

        //-------------------------------------------------------------------------
        // Draw text
        //-------------------------------------------------------------------------

        {
            EE_PROFILE_SCOPE_RENDER( "DebugDrawing::DrawText" );
            m_textRS.SetState( renderContext, viewport );

            auto textRenderfunc = [this] ( RenderContext const& renderContext, Viewport const& viewport, TVector<TextCommand> const& commands, IntRange cmdRange ) { DebugRenderer::DrawText( renderContext, viewport, commands, cmdRange ); };

            renderContext.SetDepthTestMode( DepthTestMode::On );
            DrawTextCommands( m_drawCommands.m_opaqueDepthOn.m_textCommands, renderContext, viewport, textRenderfunc );
            DrawTextCommands( m_drawCommands.m_transparentDepthOn.m_textCommands, renderContext, viewport, textRenderfunc );

            renderContext.SetDepthTestMode( DepthTestMode::Off );
            DrawTextCommands( m_drawCommands.m_opaqueDepthOff.m_textCommands, renderContext, viewport, textRenderfunc );
            DrawTextCommands( m_drawCommands.m_transparentDepthOff.m_textCommands, renderContext, viewport, textRenderfunc );
        }
    }
}
#endif