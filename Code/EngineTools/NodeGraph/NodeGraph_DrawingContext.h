#pragma once

#include "EngineTools/_Module/API.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::NodeGraph
{
    //-------------------------------------------------------------------------
    // Drawing Context
    //-------------------------------------------------------------------------
    // NOTE: There are 3 space in play

    //   * Canvas Space - Unscaled window space (relative to the ImGui window)
    //   * Window Space - Canvas space with a view scale applied
    //   * Screen Space - Relative to screen (used for imDrawlist)
    //-------------------------------------------------------------------------

    enum class DrawChannel
    {
        Background = 1,
        // Empty user channel
        ContentBackground = 3,
        // Empty user channel
        Foreground = 5,

        NumChannels,
    };

    struct DrawContext
    {
        friend class GraphView;

    public:

        // Scaling and Conversion Function
        //-------------------------------------------------------------------------

        // Apply scaling
        EE_FORCE_INLINE ImVec2 CanvasToWindow( ImVec2 const& vec ) const
        {
            return ImVec2( vec.x * m_viewScaleFactor, vec.y * m_viewScaleFactor );
        }

        // Remove scaling
        EE_FORCE_INLINE ImVec2 WindowToCanvas( ImVec2 const& vec ) const
        {
            return ImVec2( vec.x * m_inverseViewScaleFactor, vec.y * m_inverseViewScaleFactor );
        }

        // Apply scaling
        EE_FORCE_INLINE float CanvasToWindow( float f ) const
        {
            return f * m_viewScaleFactor;
        }

        // Remove scaling
        EE_FORCE_INLINE float WindowToCanvas( float f ) const
        {
            return f * m_inverseViewScaleFactor;
        }

        // Convert from a position relative to the window TL to a position relative to the screen TL
        EE_FORCE_INLINE ImVec2 WindowToScreenPosition( ImVec2 const& windowPosition ) const
        {
            return windowPosition + m_windowRect.Min;
        }

        // Convert from a position relative to the window TL to a position relative to the canvas origin
        EE_FORCE_INLINE ImVec2 WindowToCanvasPosition( ImVec2 const& windowPosition ) const
        {
            return ( windowPosition * m_inverseViewScaleFactor ) + m_viewOffset;
        }

        // Convert from a position relative to the canvas origin to a position relative to the window TL
        EE_FORCE_INLINE ImVec2 CanvasToWindowPosition( ImVec2 const& canvasPosition ) const
        {
            return ( canvasPosition - m_viewOffset ) * m_viewScaleFactor;
        }

        // Convert from a position relative to the canvas origin to a position relative to the screen TL
        EE_FORCE_INLINE ImVec2 CanvasToScreenPosition( ImVec2 const& canvasPosition ) const
        {
            ImVec2 const windowPosition = CanvasToWindowPosition( canvasPosition );
            return WindowToScreenPosition( windowPosition );
        }

        // Convert from a position relative to the screen TL to a position relative to the window TL
        EE_FORCE_INLINE ImVec2 ScreenToWindowPosition( ImVec2 const& screenPosition ) const
        {
            return screenPosition - m_windowRect.Min;
        }

        // Convert from a position relative to the screen TL to a position relative to the canvas origin
        EE_FORCE_INLINE ImVec2 ScreenToCanvasPosition( ImVec2 const& screenPosition ) const
        {
            ImVec2 const windowPosition = ScreenToWindowPosition( screenPosition );
            return WindowToCanvasPosition( windowPosition );
        }

        // Apply scaling
        EE_FORCE_INLINE ImRect CanvasToWindow( ImRect const& canvasRect ) const
        {
            ImRect const windowRect( CanvasToWindowPosition( canvasRect.Min ), CanvasToWindowPosition( canvasRect.Max ) );
            return windowRect;
        }

        // Remove scaling
        EE_FORCE_INLINE ImRect WindowToCanvas( ImRect const& windowRect ) const
        {
            ImRect const canvasRect( WindowToCanvasPosition( windowRect.Min ), WindowToCanvasPosition( windowRect.Max ) );
            return canvasRect;
        }

        // Misc Utils
        //-------------------------------------------------------------------------

        // Is a supplied rect within the canvas visible area
        EE_FORCE_INLINE bool IsItemVisible( ImRect const& itemCanvasRect ) const
        {
            return m_canvasVisibleRect.Overlaps( itemCanvasRect );
        }

        // Set the draw list channel to use
        EE_FORCE_INLINE void SetDrawChannel( uint8_t channelIndex ) const
        {
            EE_ASSERT( m_areChannelsSplit );
            m_channelSplitter.SetCurrentChannel( m_pDrawList, channelIndex ); 
        }

    private:

        // Split the draw list channels
        void SplitDrawChannels() const
        {
            EE_ASSERT( !m_areChannelsSplit );
            m_channelSplitter.Split( m_pDrawList, (uint8_t) DrawChannel::NumChannels );
            m_areChannelsSplit = true;
        }

        // Merge all the draw list channels together
        void MergeDrawChannels() const
        {
            EE_ASSERT( m_areChannelsSplit );
            m_channelSplitter.Merge( m_pDrawList );
            m_areChannelsSplit = false;
        }

        // Set the current view scale
        void SetViewScaleFactor( float viewScaleFactor )
        {
            EE_ASSERT( viewScaleFactor != 0.0f );
            m_viewScaleFactor = viewScaleFactor;
            m_inverseViewScaleFactor = 1.0f / m_viewScaleFactor;
        }

    public:

        ImDrawList*                 m_pDrawList = nullptr;
        ImVec2                      m_viewOffset = ImVec2( 0, 0 );
        ImRect                      m_windowRect;
        ImRect                      m_canvasVisibleRect;
        ImVec2                      m_mouseScreenPos = ImVec2( 0, 0 );
        ImVec2                      m_mouseCanvasPos = ImVec2( 0, 0 );
        float                       m_viewScaleFactor = 1.0f;
        float                       m_inverseViewScaleFactor = 1.0f;
        bool                        m_isReadOnly = false;

    private:

        mutable ImDrawListSplitter  m_channelSplitter;
        mutable bool                m_areChannelsSplit = false;
    };
}