#pragma once
#include "Animation_ToolsGraphNode.h"
#include "Engine/Animation/AnimationBlender.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class LocalLayerToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( LocalLayerToolsNode );

        friend class LayerBlendToolsNode;

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Special; }
        virtual char const* GetTypeName() const override { return "Local Layer"; }
        virtual char const* GetCategory() const override { return "Animation/Layers"; }
        virtual ImColor GetTitleBarColor() const override { return ImGuiX::ConvertColor( Colors::Tomato ); }
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual bool IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const override;

    private:

        EE_EXPOSE bool                              m_isSynchronized = false;
        EE_EXPOSE bool                              m_ignoreEvents = false;
        EE_EXPOSE PoseBlendMode                     m_blendMode;
    };

    //-------------------------------------------------------------------------

    class StateMachineLayerToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( StateMachineLayerToolsNode );

        friend class LayerBlendToolsNode;

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Special; }
        virtual char const* GetTypeName() const override { return "State Machine Layer"; }
        virtual char const* GetCategory() const override { return "Animation/Layers"; }
        virtual ImColor GetTitleBarColor() const override { return ImGuiX::ConvertColor( Colors::Tomato ); }
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual bool IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const override;

    private:

        EE_EXPOSE bool                              m_isSynchronized = false;
        EE_EXPOSE bool                              m_ignoreEvents = false;
        EE_EXPOSE PoseBlendMode                     m_blendMode;
    };

    //-------------------------------------------------------------------------

    class LayerBlendToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( LayerBlendToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

    private:

        virtual GraphValueType GetValueType() const override { return GraphValueType::Pose; }
        virtual char const* GetTypeName() const override { return "Layer Blend"; }
        virtual char const* GetCategory() const override { return "Animation/Layers"; }
        virtual ImColor GetTitleBarColor() const override { return ImGuiX::ConvertColor( Colors::Tomato ); }

        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual bool IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const override;

        virtual bool SupportsDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override;
        virtual uint32_t GetDynamicInputPinValueType() const override { return (uint32_t) GraphValueType::Special; }
        virtual void OnDynamicPinDestruction( UUID pinID ) override;

        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_EXPOSE bool                              m_onlySampleBaseRootMotion = true;
    };
}