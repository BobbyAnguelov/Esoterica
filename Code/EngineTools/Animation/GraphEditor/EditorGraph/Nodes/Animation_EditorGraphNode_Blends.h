#pragma once
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_FlowGraph.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Blends.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class ParameterizedBlendEditorNode : public EditorGraphNode
    {
        EE_REGISTER_TYPE( ParameterizedBlendEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetCategory() const override { return "Blends"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override final { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual bool SupportsDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override { return "Input"; }
        virtual uint32_t GetDynamicInputPinValueType() const override { return (uint32_t) GraphValueType::Pose; }

    protected:

        bool CompileParameterAndSourceNodes( GraphCompilationContext& context, ParameterizedBlendNode::Settings* pSettings ) const;

    protected:

        EE_EXPOSE bool                 m_isSynchronized = false;
    };

    //-------------------------------------------------------------------------

    class RangedBlendEditorNode final : public ParameterizedBlendEditorNode
    {
        EE_REGISTER_TYPE( RangedBlendEditorNode );

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Ranged Blend"; }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual bool DrawPinControls( VisualGraph::Flow::Pin const& pin ) override;
        virtual void OnDynamicPinCreation( UUID pinID ) override;
        virtual void OnDynamicPinDestruction( UUID pinID ) override;

    private:

        EE_REGISTER TVector<float>     m_parameterValues;
    };

    //-------------------------------------------------------------------------

    class VelocityBlendEditorNode final : public ParameterizedBlendEditorNode
    {
        EE_REGISTER_TYPE( VelocityBlendEditorNode );

        virtual char const* GetTypeName() const override { return "Velocity Blend"; }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual bool IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const override;
    };
}