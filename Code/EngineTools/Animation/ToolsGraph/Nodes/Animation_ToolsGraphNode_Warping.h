#pragma once

#include "Animation_ToolsGraphNode.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Warping.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class OrientationWarpToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( OrientationWarpToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;
        virtual bool IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const override;
        virtual char const* GetTypeName() const override { return "Orientation Warp"; }
        virtual char const* GetCategory() const override { return "Motion Warping"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class TargetWarpToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( TargetWarpToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;
        virtual bool IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const override;
        virtual char const* GetTypeName() const override { return "Target Warp"; }
        virtual char const* GetCategory() const override { return "Motion Warping"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_EXPOSE bool                             m_allowTargetUpdate = false;
        EE_EXPOSE TargetWarpNode::SamplingMode     m_samplingMode;
        EE_EXPOSE float                            m_samplingPositionErrorThreshold = 0.05f;
    };
}