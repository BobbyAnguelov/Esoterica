#pragma once
#include "Animation_ToolsGraphNode.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_CachedValues.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class CachedBoolToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( CachedBoolToolsNode );

    public:

        CachedBoolToolsNode();

        virtual char const* GetTypeName() const override { return "Cached Bool"; }
        virtual char const* GetCategory() const override { return "Values/Cached"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;

    private:

        EE_REFLECT() CachedValueMode          m_mode = CachedValueMode::OnEntry;
    };

    //-------------------------------------------------------------------------

    class CachedIDToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( CachedIDToolsNode );

    public:

        CachedIDToolsNode();

        virtual char const* GetTypeName() const override { return "Cached ID"; }
        virtual char const* GetCategory() const override { return "Values/Cached"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;

    private:

        EE_REFLECT() CachedValueMode          m_mode = CachedValueMode::OnEntry;
    };

    //-------------------------------------------------------------------------

    class CachedFloatToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( CachedFloatToolsNode );

    public:

        CachedFloatToolsNode();

        virtual char const* GetTypeName() const override { return "Cached Float"; }
        virtual char const* GetCategory() const override { return "Values/Cached"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;

    private:

        EE_REFLECT() CachedValueMode          m_mode = CachedValueMode::OnEntry;
    };

    //-------------------------------------------------------------------------

    class CachedVectorToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( CachedVectorToolsNode );

    public:

        CachedVectorToolsNode();

        virtual char const* GetTypeName() const override { return "Cached Vector"; }
        virtual char const* GetCategory() const override { return "Values/Cached"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;

    private:

        EE_REFLECT() CachedValueMode          m_mode = CachedValueMode::OnEntry;
    };

    //-------------------------------------------------------------------------

    class CachedTargetToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( CachedTargetToolsNode );

    public:

        CachedTargetToolsNode();

        virtual char const* GetTypeName() const override { return "Cached Target"; }
        virtual char const* GetCategory() const override { return "Values/Cached"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;

    private:

        EE_REFLECT() CachedValueMode          m_mode = CachedValueMode::OnEntry;
    };
}