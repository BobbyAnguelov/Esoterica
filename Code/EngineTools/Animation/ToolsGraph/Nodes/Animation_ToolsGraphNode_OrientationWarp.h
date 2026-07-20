#pragma once

#include "Animation_ToolsGraphNode.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_TargetWarp.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class OrientationWarpToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( OrientationWarpToolsNode );

        enum class OffsetType
        {
            EE_REFLECT_ENUM

            RelativeToCharacter,
            RelativeToOriginalRootMotion
        };

    public:

        OrientationWarpToolsNode();
        virtual bool IsValidConnection( UUID const& inputPinID, FlowNode const* pOutputPinNode, UUID const& outputPinID ) const override;

        virtual char const* GetTypeName() const override { return "Orientation Warp"; }
        virtual char const* GetCategory() const override { return "Animation/Root Motion"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        // The coordinate space of the supplied offset
        EE_REFLECT() OffsetType                        m_offsetType = OffsetType::RelativeToCharacter;

        // The sampling mode for the warped motion
        EE_REFLECT() RootMotionData::SamplingMode      m_samplingMode = RootMotionData::SamplingMode::WorldSpace;
    };
}