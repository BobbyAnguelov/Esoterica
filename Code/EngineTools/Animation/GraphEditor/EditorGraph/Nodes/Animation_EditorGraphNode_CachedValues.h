#pragma once
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_FlowGraph.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_CachedValues.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class CachedBoolEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( CachedBoolEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Cached Bool"; }
        virtual char const* GetCategory() const override { return "Values/Bool"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    private:

        EE_EXPOSE CachedValueMode          m_mode = CachedValueMode::OnEntry;
    };

    //-------------------------------------------------------------------------

    class CachedIDEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( CachedIDEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Cached ID"; }
        virtual char const* GetCategory() const override { return "Values/ID"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    private:

        EE_EXPOSE CachedValueMode          m_mode = CachedValueMode::OnEntry;
    };

    //-------------------------------------------------------------------------
    
    class CachedIntEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( CachedIntEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Cached Int"; }
        virtual char const* GetCategory() const override { return "Values/Int"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    private:

        EE_EXPOSE CachedValueMode          m_mode = CachedValueMode::OnEntry;
    };

    //-------------------------------------------------------------------------

    class CachedFloatEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( CachedFloatEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Cached Float"; }
        virtual char const* GetCategory() const override { return "Values/Float"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    private:

        EE_EXPOSE CachedValueMode          m_mode = CachedValueMode::OnEntry;
    };

    //-------------------------------------------------------------------------

    class CachedVectorEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( CachedVectorEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Cached Vector"; }
        virtual char const* GetCategory() const override { return "Values/Vector"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    private:

        EE_EXPOSE CachedValueMode          m_mode = CachedValueMode::OnEntry;
    };

    //-------------------------------------------------------------------------

    class CachedTargetEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( CachedTargetEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Cached Target"; }
        virtual char const* GetCategory() const override { return "Values/Target"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    private:

        EE_EXPOSE CachedValueMode          m_mode = CachedValueMode::OnEntry;
    };
}