#pragma once
#include "Animation_ToolsGraphNode.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Layers.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class LocalLayerToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( LocalLayerToolsNode );

        friend class LayerBlendToolsNode;

    public:

        LocalLayerToolsNode();

        virtual char const* GetTypeName() const override { return "Local Layer"; }
        virtual char const* GetCategory() const override { return "Animation/Layers"; }
        virtual Color GetTitleBarColor() const override { return Colors::Tomato; }
        virtual void DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;

        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual bool IsValidConnection( UUID const& inputPinID, FlowNode const* pOutputPinNode, UUID const& outputPinID ) const override;

    private:

        EE_REFLECT() bool                              m_isSynchronized = false;
        EE_REFLECT() bool                              m_ignoreEvents = false;
        EE_REFLECT() PoseBlendMode                     m_blendMode = PoseBlendMode::Overlay;
    };

    //-------------------------------------------------------------------------

    class StateMachineLayerToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( StateMachineLayerToolsNode );

        friend class LayerBlendToolsNode;

    public:

        StateMachineLayerToolsNode();

        virtual char const* GetTypeName() const override { return "State Machine Layer"; }
        virtual char const* GetCategory() const override { return "Animation/Layers"; }
        virtual Color GetTitleBarColor() const override { return Colors::Tomato; }
        void DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;
        virtual void DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;

        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual bool IsValidConnection( UUID const& inputPinID, FlowNode const* pOutputPinNode, UUID const& outputPinID ) const override;

    private:

        EE_REFLECT() bool                              m_isSynchronized = false;
        EE_REFLECT() bool                              m_ignoreEvents = false;
        EE_REFLECT() PoseBlendMode                     m_blendMode = PoseBlendMode::Overlay;

        float                                          m_runtimeDebugLayerWeight = 0.0f;
    };

    //-------------------------------------------------------------------------

    class LayerBlendToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( LayerBlendToolsNode );

    public:

        LayerBlendToolsNode();

    private:

        virtual char const* GetTypeName() const override { return "Layer Blend"; }
        virtual char const* GetCategory() const override { return "Animation/Layers"; }
        virtual Color GetTitleBarColor() const override { return Colors::Tomato; }

        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual bool IsValidConnection( UUID const& inputPinID, FlowNode const* pOutputPinNode, UUID const& outputPinID ) const override;

        virtual bool SupportsUserEditableDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override;
        virtual StringID GetDynamicInputPinValueType() const override { return GetPinTypeForValueType( GraphValueType::Special ); }
        virtual void PreDynamicPinDestruction( UUID const& pinID ) override;
        virtual void PostDynamicPinDestruction() override;

        virtual int16_t Compile( GraphCompilationContext& context ) const override;

        virtual void PreDrawUpdate( NodeGraph::UserContext* pUserContext ) override;

    private:

        EE_REFLECT() bool                              m_onlySampleBaseRootMotion = true;
    };
}