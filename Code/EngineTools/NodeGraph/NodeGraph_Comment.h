#pragma once

#include "EngineTools/_Module/API.h"
#include "NodeGraph_BaseGraph.h"

//-------------------------------------------------------------------------
// Comment Node
//-------------------------------------------------------------------------

namespace EE::NodeGraph
{
    class EE_ENGINETOOLS_API CommentNode final : public BaseNode
    {
        friend class GraphView;

        EE_REFLECT_TYPE( CommentNode );

    public:

        using BaseNode::BaseNode;

        constexpr static float const s_resizeSelectionRadius = 10.0f;
        constexpr static float const s_minBoxDimensions = ( s_resizeSelectionRadius * 2 ) + 20.0f;

    protected:

        virtual char const* GetTypeName() const override { return "Comment"; }

        virtual bool IsRenameable() const override { return true; }

        Color GetCommentBoxColor() const { return m_nodeColor; }

        // Draw the context menu options - returns true if the menu should be closed i.e. a custom selection or action has been made
        bool DrawContextMenuOptions( DrawContext const& ctx, UserContext* pUserContext, Float2 const& mouseCanvasPos );

        // Get the currently hovered resize handle
        ResizeHandle GetHoveredResizeHandle( DrawContext const& ctx ) const;

        // Get the comment box size (this is the actual full node size in canvas units - including title)
        Float2 const& GetCommentBoxSize() const { return m_commentBoxSize; }

    private:

        CommentNode( CommentNode const& ) = delete;
        CommentNode& operator=( CommentNode const& ) = delete;

        void AdjustSizeBasedOnMousePosition( DrawContext const& ctx, ResizeHandle handle );

    private:

        EE_REFLECT( Hidden );
        Float2                      m_commentBoxSize = Float2( 0, 0 );

        EE_REFLECT( Hidden );
        Color                       m_nodeColor = Color( 0xFF4C4C4C );
    };
}