#pragma once
#include "Animation_ToolsGraphNode.h"
#include "Engine/Animation/AnimationBlender.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class LayerSettingsToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( LayerSettingsToolsNode );

        friend class LayerToolsNode;

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Layer"; }
        virtual char const* GetCategory() const override { return "Blends/Layers"; }
        virtual ImColor GetTitleBarColor() const override { return ImGuiX::ConvertColor( Colors::Pink ); }

        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual bool IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const override;

    private:

        EE_EXPOSE bool                             m_isSynchronized = false;
        EE_EXPOSE bool                             m_ignoreEvents = false;
        EE_EXPOSE TBitFlags<PoseBlendOptions>      m_blendOptions;
    };

    //-------------------------------------------------------------------------

    class LayerToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( LayerToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

    private:

        virtual char const* GetTypeName() const override { return "Layer Blend"; }
        virtual char const* GetCategory() const override { return "Blends/Layers"; }
        virtual ImColor GetTitleBarColor() const override { return ImGuiX::ConvertColor( Colors::Pink ); }

        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual bool IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const override;

        virtual bool SupportsDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override;
        virtual uint32_t GetDynamicInputPinValueType() const override { return (uint32_t) GraphValueType::Unknown; }
        virtual void OnDynamicPinDestruction( UUID pinID ) override;

        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_EXPOSE bool                             m_onlySampleBaseRootMotion = true;
    };
}