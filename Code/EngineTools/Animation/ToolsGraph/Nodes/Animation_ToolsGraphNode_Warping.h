#pragma once

#include "Animation_ToolsGraphNode.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_TargetWarp.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
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

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;
        virtual bool IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Pose; }
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

    //-------------------------------------------------------------------------

    class TargetWarpToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( TargetWarpToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;
        virtual bool IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Pose; }
        virtual char const* GetTypeName() const override { return "Target Warp"; }
        virtual char const* GetCategory() const override { return "Animation/Root Motion"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        // Can this warp be updated when the target changes?
        EE_REFLECT() bool                              m_allowTargetUpdate = false;

        // The sampling mode for the warped motion
        EE_REFLECT() RootMotionData::SamplingMode      m_samplingMode = RootMotionData::SamplingMode::WorldSpace;

        // What's the error threshold we need to exceed, when accurately sampling, before we switch to inaccurate sampling
        EE_REFLECT() float                             m_samplingPositionErrorThreshold = 0.05f;

        // Length limit on generated Bezier/Hermite warp curve tangents. The lower the length the lower the curvature.
        EE_REFLECT() float                             m_maxTangentLength = 1.25f;

        // The distance under which we fallback to LERPing the XY translation instead of generating a curve
        EE_REFLECT() float                             m_lerpFallbackDistanceThreshold = 0.1f;

        // The difference between the new target and the original target before we update the warp
        EE_REFLECT() float                             m_targetUpdateDistanceThreshold = 0.1f;

        // The difference between the new target and the original target before we update the warp
        EE_REFLECT() Degrees                           m_targetUpdateAngleThreshold = 5.0f;
    };
}