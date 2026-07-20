#pragma once

#include "Animation_ToolsGraphNode_AnimationClip.h"
#include "Animation_ToolsGraphNode_Result.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class TargetSelectorToolsNode : public FlowToolsNode
    {
        EE_REFLECT_TYPE( TargetSelectorToolsNode );

    public:

        TargetSelectorToolsNode();

        inline String const& GetOptionLabel( int32_t optionIdx ) const { return m_optionLabels[optionIdx]; }

    private:

        virtual char const* GetTypeName() const override { return "Target Selector"; }
        virtual char const* GetCategory() const override { return "Animation/Selectors"; }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual bool IsValidConnection( UUID const& inputPinID, FlowNode const* pOutputPinNode, UUID const& outputPinID ) const override;
        virtual bool IsAnimationClipReferenceNode() const override { return true; }
        virtual bool IsRenameable() const override { return true; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }

        virtual bool SupportsUserEditableDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override { return "Option"; }
        virtual StringID GetDynamicInputPinValueType() const override { return GetPinTypeForValueType( GraphValueType::Pose ); }
        virtual void OnDynamicPinCreation( UUID const& pinID ) override;
        virtual void PreDynamicPinDestruction( UUID const& pinID ) override;

    protected:

        virtual void PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited ) override;
        virtual void RefreshDynamicPins() override;

    private:

        EE_REFLECT( ShowAsStaticArray );
        TVector<String>             m_optionLabels;

        // How important is orientation to the selection
        EE_REFLECT( Min = "0.0", Max = "1.0" );
        float                       m_orientationScoreWeight = 1.0f;

        // How important is position to the selection
        EE_REFLECT( Min = "0.0", Max = "1.0" );
        float                       m_positionScoreWeight = 1.0f;

        EE_REFLECT();
        bool                        m_isWorldSpaceTarget = false;

        EE_REFLECT( Category = "Advanced" );
        bool                        m_ignoreInvalidOptions = false;
    };
}