#pragma once

#include "Base/Types/Arrays.h"
#include "Base/Math/Math.h"

//-------------------------------------------------------------------------
// Tree Layout Tool
//-------------------------------------------------------------------------
// Usage:   Fill out tree nodes and call "PerformLayout"
//          This will calculate the final positions of all the nodes as well the tree dimensions

namespace EE
{
    class EE_BASE_API TreeLayout
    {
    public:

        struct Node_t
        {
            inline int32_t GetNumChildren() const { return (int32_t) m_children.size(); }
            void CalculateRelativePositions( Float2 const &nodeSpacing );
            void CalculateAbsolutePositions( Float2 const &parentOffset );

            template<typename T>
            T const *GetItem() const { return reinterpret_cast<T const *>( m_pItem ); }

            inline Float2 GetTopLeft() const { return m_position - Float2( m_width / 2, m_height / 2 ); }
            inline Float2 GetBottomRight() const { return m_position + Float2( m_width / 2, m_height / 2 ); }

        public:

            void                            *m_pItem = nullptr;
            Float2                          m_relativePosition = Float2::Zero;
            Float2                          m_position = Float2::Zero; // Absolute center position
            float                           m_width = 0; // desired node width
            float                           m_height = 0; // desired node height
            float                           m_combinedWidth = 0.0f; // Total width for this node and all children
            TVector<Node_t*>                m_children;
        };

    public:

        inline bool HasNodes() const { return !m_nodes.empty(); }

        int32_t GetNodeIndex( Node_t const* pNode ) const;

        void PerformLayout( Float2 const &nodeSpacing, Float2 const &rootNodePos = Float2::Zero );

    public:

        TInlineVector<Node_t, 16>           m_nodes;
        Float2                              m_dimensions = Float2::Zero;
    };
}