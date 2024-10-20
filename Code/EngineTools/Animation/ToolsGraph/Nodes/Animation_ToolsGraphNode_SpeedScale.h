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

        virtual char const* GetTypeName() const override { return "Speed Scale"; }
        virtual char const* GetCategory() const override { return "Animation/Speed Scale"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( NodeGraph::DrawContext const& ctx ) override;

    private:

        EE_REFLECT();
        float                   m_multiplier = 1.0f; // The desired speed multiplier if the input node is not set
    };

    //-------------------------------------------------------------------------

    class DurationScaleToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( DurationScaleToolsNode );

    public:

        DurationScaleToolsNode();

        virtual char const* GetTypeName() const override { return "Duration Scale"; }
        virtual char const* GetCategory() const override { return "Animation/Speed Scale"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( NodeGraph::DrawContext const& ctx ) override;

    private:

        EE_REFLECT();
        float                   m_desiredDuration = 1.0f; // The desired duration if the duration input node is not set
    };

    //-------------------------------------------------------------------------

    class VelocityBasedSpeedScaleToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( VelocityBasedSpeedScaleToolsNode );

    public:

        VelocityBasedSpeedScaleToolsNode();

        virtual char const* GetTypeName() const override { return "Velocity Based Speed Scale"; }
        virtual char const* GetCategory() const override { return "Animation/Speed Scale"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual bool IsValidConnection( UUID const& inputPinID, FlowNode const* pOutputPinNode, UUID const& outputPinID ) const override;
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( NodeGraph::DrawContext const& ctx ) override;

    private:

        EE_REFLECT();
        float                   m_desiredVelocity = 1.0f; // The desired velocity if the speed input node is not set
    };
}