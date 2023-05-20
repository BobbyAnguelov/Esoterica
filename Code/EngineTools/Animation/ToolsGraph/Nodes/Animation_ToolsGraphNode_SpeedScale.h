#pragma once

#include "Animation_ToolsGraphNode.h"
#include "System/Math/NumericRange.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class SpeedScaleToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( SpeedScaleToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Pose; }
        virtual char const* GetTypeName() const override { return "Speed Scale"; }
        virtual char const* GetCategory() const override { return "Animation/Speed"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_REFLECT() FloatRange              m_scaleLimits = FloatRange( 0.01f, 10.0f );
        EE_REFLECT() float                   m_blendTime = 0.2f; // How long should we take to initially blend in this speed modification
    };

    //-------------------------------------------------------------------------

    class VelocityBasedSpeedScaleToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( VelocityBasedSpeedScaleToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Pose; }
        virtual char const* GetTypeName() const override { return "Velocity Based Speed Scale"; }
        virtual char const* GetCategory() const override { return "Animation/Speed"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_REFLECT() float                   m_blendTime = 0.2f; // How long should we take to initially blend in this speed modification
    };
}