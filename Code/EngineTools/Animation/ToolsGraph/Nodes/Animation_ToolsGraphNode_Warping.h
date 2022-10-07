#pragma once

#include "Animation_ToolsGraphNode.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_TargetWarp.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class OrientationWarpToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( OrientationWarpToolsNode );

        enum class OffsetType
        {
            EE_REGISTER_ENUM

            RelativeToCharacter,
            RelativeToOriginalRootMotion
        };

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;
        virtual bool IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const override;
        virtual char const* GetTypeName() const override { return "Orientation Warp"; }
        virtual char const* GetCategory() const override { return "Motion Warping"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_EXPOSE OffsetType m_offsetType = OffsetType::RelativeToCharacter;
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

        // Can this warp be updated when the target changes?
        EE_EXPOSE bool                             m_allowTargetUpdate = false;

        // The sampling mode for the warped motion
        EE_EXPOSE TargetWarpNode::SamplingMode     m_samplingMode;

        // What's the error threshold we need to exceed, when accurately sampling, before we switch to inaccurate sampling
        EE_EXPOSE float                            m_samplingPositionErrorThreshold = 0.05f;
    };
}