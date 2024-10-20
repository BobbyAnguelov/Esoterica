#pragma once
#include "Animation_ToolsGraphNode.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Targets.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class IsTargetSetToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( IsTargetSetToolsNode );

    public:

        IsTargetSetToolsNode();

        virtual char const* GetTypeName() const override { return "Is Target Set"; }
        virtual char const* GetCategory() const override { return "Values/Target"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class TargetInfoToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( TargetInfoToolsNode );

    public:

        TargetInfoToolsNode();

        virtual char const* GetTypeName() const override { return "Target Info"; }
        virtual char const* GetCategory() const override { return "Values/Target"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( NodeGraph::DrawContext const& ctx ) override;

    private:

        EE_REFLECT() TargetInfoNode::Info     m_infoType = TargetInfoNode::Info::Distance;
        EE_REFLECT() bool                     m_isWorldSpaceTarget = true;
    };

    //-------------------------------------------------------------------------

    class TargetPointToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( TargetPointToolsNode );

    public:

        TargetPointToolsNode();

        virtual char const* GetTypeName() const override { return "Target Point"; }
        virtual char const* GetCategory() const override { return "Values/Target"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_REFLECT() bool                     m_isWorldSpaceTarget = true;
    };

    //-------------------------------------------------------------------------

    class TargetOffsetToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( TargetOffsetToolsNode );

    public:

        TargetOffsetToolsNode();

        virtual char const* GetTypeName() const override { return "Target Offset"; }
        virtual char const* GetCategory() const override { return "Values/Target"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_REFLECT() bool                 m_isBoneSpaceOffset  = true;
        EE_REFLECT() Quaternion           m_rotationOffset = Quaternion::Identity;
        EE_REFLECT() Vector               m_translationOffset = Vector::Zero;
    };
}