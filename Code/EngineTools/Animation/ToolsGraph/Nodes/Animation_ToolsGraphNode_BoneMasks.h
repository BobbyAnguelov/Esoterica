#pragma once
#include "Animation_ToolsGraphNode_DataSlot.h"
#include "Engine/Animation/AnimationBoneMask.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class BoneMaskToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( BoneMaskToolsNode );

    public:

        BoneMaskToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::BoneMask; }
        virtual char const* GetTypeName() const override { return "Bone Mask"; }
        virtual char const* GetCategory() const override { return "Values/Bone Mask"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::ValueTree, GraphType::TransitionTree, GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;
        virtual void OnDoubleClick( VisualGraph::UserContext* pUserContext ) override;

    private:

        EE_REFLECT( "CustomEditor" : "AnimGraph_BoneMaskID" );
        StringID                               m_maskID;
    };

    //-------------------------------------------------------------------------

    class FixedWeightBoneMaskToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( FixedWeightBoneMaskToolsNode );

    public:

        FixedWeightBoneMaskToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::BoneMask; }
        virtual char const* GetTypeName() const override { return "Fixed Weight Bone Mask"; }
        virtual char const* GetCategory() const override { return "Values/Bone Mask"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::ValueTree, GraphType::TransitionTree, GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

        EE_REFLECT();
        float                                   m_boneWeight;
    };

    //-------------------------------------------------------------------------

    class BoneMaskBlendToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( BoneMaskBlendToolsNode );

    public:

        BoneMaskBlendToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::BoneMask; }
        virtual char const* GetTypeName() const override { return "Bone Mask Blend"; }
        virtual char const* GetCategory() const override { return "Values/Bone Mask"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::ValueTree, GraphType::TransitionTree, GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class BoneMaskSelectorToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( BoneMaskSelectorToolsNode );

    public:

        BoneMaskSelectorToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::BoneMask; }
        virtual char const* GetTypeName() const override { return "Bone Mask Selector"; }
        virtual char const* GetCategory() const override { return "Values/Bone Mask"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::ValueTree, GraphType::TransitionTree, GraphType::BlendTree ); }

        virtual int16_t Compile( GraphCompilationContext& context ) const override;

        virtual bool SupportsUserEditableDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override;
        virtual StringID GetDynamicInputPinValueType() const override { return GetPinTypeForValueType( GraphValueType::BoneMask ); }
        virtual void OnDynamicPinCreation( UUID pinID ) override;
        virtual void OnDynamicPinDestruction( UUID pinID ) override;

        virtual void GetLogicAndEventIDs( TVector<StringID>& outIDs ) const override;
        virtual void RenameLogicAndEventIDs( StringID oldID, StringID newID ) override;

    private:

        EE_REFLECT();
        bool                                   m_switchDynamically = false;

        EE_REFLECT();
        TVector<StringID>                      m_parameterValues;

        EE_REFLECT();
        Seconds                                m_blendTime = 0.1f;
    };
}