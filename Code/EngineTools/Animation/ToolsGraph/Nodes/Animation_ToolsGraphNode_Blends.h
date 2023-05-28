#pragma once
#include "Animation_ToolsGraphNode.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Blends.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class ParameterizedBlendToolsNode : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ParameterizedBlendToolsNode );

    public:

        ParameterizedBlendToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Pose; }
        virtual char const* GetCategory() const override { return "Animation/Blends"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override final { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual bool SupportsUserEditableDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override { return "Input"; }
        virtual uint32_t GetDynamicInputPinValueType() const override { return (uint32_t) GraphValueType::Pose; }

    protected:

        bool CompileParameterAndSourceNodes( GraphCompilationContext& context, ParameterizedBlendNode::Settings* pSettings ) const;

    protected:

        EE_REFLECT()
        bool                 m_isSynchronized = false;
    };

    //-------------------------------------------------------------------------

    class RangedBlendToolsNode final : public ParameterizedBlendToolsNode
    {
        EE_REFLECT_TYPE( RangedBlendToolsNode );

        RangedBlendToolsNode();

        virtual char const* GetTypeName() const override { return "Animation/Ranged Blend"; }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual bool DrawPinControls( VisualGraph::UserContext* pUserContext, VisualGraph::Flow::Pin const& pin ) override;
        virtual void OnDynamicPinCreation( UUID pinID ) override;
        virtual void OnDynamicPinDestruction( UUID pinID ) override;

    private:

        EE_REFLECT( "IsToolsReadOnly" : true )
        TVector<float>     m_parameterValues;
    };

    //-------------------------------------------------------------------------

    class VelocityBlendToolsNode final : public ParameterizedBlendToolsNode
    {
        EE_REFLECT_TYPE( VelocityBlendToolsNode );

        virtual char const* GetTypeName() const override { return "Animation/Velocity Blend"; }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual bool IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const override;
    };
}