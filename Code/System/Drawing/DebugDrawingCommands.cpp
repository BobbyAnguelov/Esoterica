#include "DebugDrawingCommands.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Drawing
{
    void CommandBuffer::Reset( Seconds deltaTime )
    {
        // NAIVE delete version - profile this

        for ( int32_t i = (int32_t) m_pointCommands.size() - 1; i >= 0; i-- )
        {
            m_pointCommands[i].m_TTL -= deltaTime;
            if ( m_pointCommands[i].m_TTL <= 0.0f )
            {
                m_pointCommands.erase_unsorted( m_pointCommands.begin() + i );
            }
        }

        for ( int32_t i = (int32_t) m_lineCommands.size() - 1; i >= 0; i-- )
        {
            m_lineCommands[i].m_TTL -= deltaTime;
            if ( m_lineCommands[i].m_TTL <= 0.0f )
            {
                m_lineCommands.erase_unsorted( m_lineCommands.begin() + i );
            }
        }

        for ( int32_t i = (int32_t) m_triangleCommands.size() - 1; i >= 0; i-- )
        {
            m_triangleCommands[i].m_TTL -= deltaTime;
            if ( m_triangleCommands[i].m_TTL <= 0.0f )
            {
                m_triangleCommands.erase_unsorted( m_triangleCommands.begin() + i );
            }
        }

        for ( int32_t i = (int32_t) m_textCommands.size() - 1; i >= 0; i-- )
        {
            m_textCommands[i].m_TTL -= deltaTime;
            if ( m_textCommands[i].m_TTL <= 0.0f )
            {
                m_textCommands.erase_unsorted( m_textCommands.begin() + i );
            }
        }
    }

    void FrameCommandBuffer::AddThreadCommands( ThreadCommandBuffer const& threadCommands )
    {
        // TODO:
        // Broad-phase culling
        // Sort transparent and depth test off primitives by distance to camera
        // Sort text by font

        m_opaqueDepthOn.Append( threadCommands.GetOpaqueDepthTestEnabledBuffer() );
        m_opaqueDepthOff.Append( threadCommands.GetOpaqueDepthTestDisabledBuffer() );
        m_transparentDepthOn.Append( threadCommands.GetTransparentDepthTestEnabledBuffer() );
        m_transparentDepthOff.Append( threadCommands.GetTransparentDepthTestDisabledBuffer() );
    }
}
#endif