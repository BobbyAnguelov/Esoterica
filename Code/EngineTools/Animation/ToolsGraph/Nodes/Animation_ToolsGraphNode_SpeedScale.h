#pragma once

#include "Animation_ToolsGraphNode.h"
#include "Base/Math/NumericRange.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class SpeedScaleToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( SpeedScaleToolsNode );

    public:

        SpeedScaleToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Pose; }
        virtual char const* GetTypeName() const override { return "Speed Scale"; }
        virtual char const* GetCategory() const override { return "Animation/Speed Scale"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_REFLECT();
        float                   m_blendInTime = 0.2f; // How long should we take to initially blend in this speed modification
    };

    //-------------------------------------------------------------------------

    class DurationScaleToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( DurationScaleToolsNode );

    public:

        DurationScaleToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Pose; }
        virtual char const* GetTypeName() const override { return "Duration Scale"; }
        virtual char const* GetCategory() const override { return "Animation/Speed Scale"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    private:

        EE_REFLECT();
        float                   m_desiredDuration = 1.0f; // The desired duration for the input
    };

    //-------------------------------------------------------------------------

    class VelocityBasedSpeedScaleToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( VelocityBasedSpeedScaleToolsNode );

    public:

        VelocityBasedSpeedScaleToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Pose; }
        virtual char const* GetTypeName() const override { return "Velocity Based Speed Scale"; }
        virtual char const* GetCategory() const override { return "Animation/Speed Scale"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_REFLECT();
        float                   m_blendTime = 0.2f; // How long should we take to initially blend in this speed modification
    };
}