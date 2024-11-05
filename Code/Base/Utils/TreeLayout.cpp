#include "TreeLayout.h"
#include "Base/Math/Vector.h"

//-------------------------------------------------------------------------

namespace EE
{
    void TreeLayout::Node_t::CalculateRelativePositions( Float2 const &nodeSpacing )
    {
        int32_t const numChildren = (int32_t) m_children.size();
        if ( numChildren == 0 )
        {
            EE_ASSERT( m_width != 0 );
            m_combinedWidth = m_width;
        }
        else if ( numChildren == 1 )
        {
            float const offsetY = ( m_height / 2 ) + ( m_children[0]->m_height / 2 ) + nodeSpacing.m_y;

            m_children[0]->CalculateRelativePositions( nodeSpacing );
            m_children[0]->m_relativePosition = Float2( 0.0f, offsetY );
            m_combinedWidth = Math::Max( m_width, m_children[0]->m_combinedWidth );
            EE_ASSERT( m_width != 0.0f );
        }
        else // multiple children
        {
            m_combinedWidth = 0.0f;

            // Calculate the relative positions of children and the overall width of this node
            //-------------------------------------------------------------------------

            for ( int32_t i = 0; i < numChildren; i++ )
            {
                m_children[i]->CalculateRelativePositions( nodeSpacing );
                m_combinedWidth += m_children[i]->m_combinedWidth;

                if ( i != 0 )
                {
                    m_combinedWidth += nodeSpacing.m_x;
                }

                EE_ASSERT( m_combinedWidth != 0.0f );
            }

            // Offset children based on total width
            //-------------------------------------------------------------------------

            float startOffset = -( m_combinedWidth / 2 );
            for ( int32_t i = 0; i < numChildren; i++ )
            {
                float const flOffsetY = ( m_height / 2 ) + ( m_children[0]->m_height / 2 ) + nodeSpacing.m_y;

                float flHalfWidth = m_children[i]->m_combinedWidth / 2;
                m_children[i]->m_relativePosition = Float2( startOffset + flHalfWidth, flOffsetY );
                startOffset += m_children[i]->m_combinedWidth + nodeSpacing.m_x;
            }

            m_combinedWidth = Math::Max( m_width, m_combinedWidth );
        }
    }

    void TreeLayout::Node_t::CalculateAbsolutePositions( Float2 const &parentOffset )
    {
        m_position = parentOffset + m_relativePosition;

        int32_t const numChildren = (int32_t) m_children.size();
        for ( int32_t i = 0; i < numChildren; i++ )
        {
            m_children[i]->CalculateAbsolutePositions( m_position );
        }
    }

    int32_t TreeLayout::GetNodeIndex( Node_t const* pNode ) const
    {
        for ( int32_t i = 0; i < (int32_t) m_nodes.size(); i++ )
        {
            if ( &m_nodes[i] == pNode )
            {
                return i;
            }
        }

        return InvalidIndex;
    }

    void TreeLayout::PerformLayout( Float2 const &nodeSpacing, Float2 const &rootNodePos )
    {
        m_nodes.back().CalculateRelativePositions( nodeSpacing );
        m_nodes.back().CalculateAbsolutePositions( rootNodePos );

        if ( m_nodes.empty() )
        {
            m_dimensions = Float2::Zero;
        }
        else
        {
            Float2 minPoint = Float2( FLT_MAX, FLT_MAX );
            Float2 maxPoint = Float2( -FLT_MAX, -FLT_MAX );

            for ( int32_t i = 0; i <  (int32_t) m_nodes.size(); i++ )
            {
                minPoint = Vector::Min( minPoint, m_nodes[i].GetTopLeft() ).ToFloat2();
                maxPoint = Vector::Max( maxPoint, m_nodes[i].GetBottomRight() ).ToFloat2();
            }

            m_dimensions = maxPoint - minPoint;
            m_dimensions += ( nodeSpacing * 2 );
        }
    }
}