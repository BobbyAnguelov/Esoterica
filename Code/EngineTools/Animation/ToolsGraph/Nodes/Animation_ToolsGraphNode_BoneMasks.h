#pragma once
#include "Animation_ToolsGraphNode_DataSlot.h"
#include "Engine/Animation/AnimationBoneMask.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class BoneMaskToolsNode final : public DataSlotToolsNode
    {
        EE_REGISTER_TYPE( BoneMaskToolsNode );

        // TEMP: until we have a better editor
        struct ToolsBoneWeight : public IRegisteredType
        {
            EE_REGISTER_TYPE( ToolsBoneWeight )

            StringID        m_boneID;
            float           m_weight;
        };

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Bone Mask"; }
        virtual char const* GetCategory() const override { return "Values/Bone Mask"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

        virtual char const* const GetDefaultSlotName() const override { return "Bone Mask"; }
        virtual ResourceTypeID GetSlotResourceTypeID() const override;

    private:

        EE_EXPOSE float                                m_rootMotionWeight = 1.0f;
    };

    //-------------------------------------------------------------------------

    class BoneMaskBlendToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( BoneMaskBlendToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Bone Mask Blend"; }
        virtual char const* GetCategory() const override { return "Values/Bone Mask"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class BoneMaskSelectorToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( BoneMaskSelectorToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Bone Mask Selector"; }
        virtual char const* GetCategory() const override { return "Values/Bone Mask"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::ValueTree, GraphType::TransitionTree ); }

        virtual bool SupportsDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override { return "Mask"; }
        virtual uint32_t GetDynamicInputPinValueType() const override { return (uint32_t) GraphValueType::BoneMask; }

        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_EXPOSE bool                                   m_switchDynamically = false;
        EE_EXPOSE TVector<StringID>                      m_parameterValues;
        EE_EXPOSE Seconds                                m_blendTime = 0.1f;
    };
}