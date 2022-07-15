#pragma once
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_FlowGraph.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Targets.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class IsTargetSetEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( IsTargetSetEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Is Target Set"; }
        virtual char const* GetCategory() const override { return "Values/Target"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class TargetInfoEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( TargetInfoEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Target Info"; }
        virtual char const* GetCategory() const override { return "Values/Target"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    private:

        EE_EXPOSE TargetInfoNode::Info     m_infoType = TargetInfoNode::Info::Distance;
        EE_EXPOSE bool                     m_isWorldSpaceTarget = true;
    };

    //-------------------------------------------------------------------------

    class TargetOffsetEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( TargetOffsetEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Target Offset"; }
        virtual char const* GetCategory() const override { return "Values/Target"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_EXPOSE bool                 m_isBoneSpaceOffset  = true;
        EE_EXPOSE Quaternion           m_rotationOffset = Quaternion::Identity;
        EE_EXPOSE Vector               m_translationOffset = Vector::Zero;
    };
}