#pragma once
#include "Animation_ToolsGraphNode_DataSlot.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class PoweredRagdollToolsNode final : public DataSlotToolsNode
    {
        EE_REFLECT_TYPE( PoweredRagdollToolsNode );

    public:

        PoweredRagdollToolsNode();

        virtual char const* GetTypeName() const override { return "Powered Ragdoll"; }
        virtual char const* GetCategory() const override { return "Animation/Physics"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

        virtual char const* GetDefaultSlotName() const override { return "Ragdoll"; }
        virtual ResourceTypeID GetSlotResourceTypeID() const override;

    private:

        EE_REFLECT() StringID                m_profileID;
        EE_REFLECT() float                   m_physicsBlendWeight = 1.0f;
        EE_REFLECT() bool                    m_isGravityEnabled = false;
    };

    //-------------------------------------------------------------------------

    class SimulatedRagdollToolsNode final : public DataSlotToolsNode
    {
        EE_REFLECT_TYPE( SimulatedRagdollToolsNode );

    public:

        SimulatedRagdollToolsNode();

        virtual char const* GetTypeName() const override { return "Simulated Ragdoll"; }
        virtual char const* GetCategory() const override { return "Animation/Physics"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

        virtual char const* GetDefaultSlotName() const override { return "Ragdoll"; }
        virtual ResourceTypeID GetSlotResourceTypeID() const override;

    private:

        virtual bool SupportsUserEditableDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override { return "Exit Option"; }
        virtual StringID GetDynamicInputPinValueType() const override { return GetPinTypeForValueType( GraphValueType::Pose ); }
        virtual bool IsValidConnection( UUID const& inputPinID, FlowNode const* pOutputPinNode, UUID const& outputPinID ) const override;

    private:

        // The profile to use when initializing the ragdoll simulation
        EE_REFLECT() StringID                m_entryProfileID;

        // The profile to use when "fully simulated"
        EE_REFLECT() StringID                m_simulatedProfileID;

        // The profile to use when leaving "fully simulated"
        EE_REFLECT() StringID                m_exitProfileID;
    };
}