#pragma once

#include "EngineTools/_Module/API.h"
#include "System/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::VisualGraph
{
    //-------------------------------------------------------------------------
    // Drawing Context
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

        // Convert from a position relative to the window TL to a position relative to the screen TL
        EE_FORCE_INLINE ImVec2 WindowToScreenPosition( ImVec2 const& windowPosition ) const
        {
            return windowPosition + m_windowRect.Min;
        }

        // Convert from a position relative to the window TL to a position relative to the canvas origin
        EE_FORCE_INLINE ImVec2 WindowPositionToCanvasPosition( ImVec2 const& windowPosition ) const
        {
            return windowPosition + m_viewOffset;
        }

        // Convert from a position relative to the canvas origin to a position relative to the window TL
        EE_FORCE_INLINE ImVec2 CanvasPositionToWindowPosition( ImVec2 const& canvasPosition ) const
        {
            return canvasPosition - m_viewOffset;
        }

        // Convert from a position relative to the canvas origin to a position relative to the screen TL
        EE_FORCE_INLINE ImVec2 CanvasPositionToScreenPosition( ImVec2 const& canvasPosition ) const
        {
            return WindowToScreenPosition( CanvasPositionToWindowPosition( canvasPosition ) );
        }

        // Convert from a position relative to the screen TL to a position relative to the window TL
        EE_FORCE_INLINE ImVec2 ScreenPositionToWindowPosition( ImVec2 const& screenPosition ) const
        {
            return screenPosition - m_windowRect.Min;
        }

        // Convert from a position relative to the screen TL to a position relative to the canvas origin
        EE_FORCE_INLINE ImVec2 ScreenPositionToCanvasPosition( ImVec2 const& screenPosition ) const
        {
            return WindowPositionToCanvasPosition( ScreenPositionToWindowPosition( screenPosition ) );
        }

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

    public:

        ImDrawList*                 m_pDrawList = nullptr;
        ImVec2                      m_viewOffset = ImVec2( 0, 0 );
        ImRect                      m_windowRect;
        ImRect                      m_canvasVisibleRect;
        ImVec2                      m_mouseScreenPos = ImVec2( 0, 0 );
        ImVec2                      m_mouseCanvasPos = ImVec2( 0, 0 );
        bool                        m_isReadOnly = false;

    private:

        mutable ImDrawListSplitter  m_channelSplitter;
        mutable bool                m_areChannelsSplit = false;
    };
}