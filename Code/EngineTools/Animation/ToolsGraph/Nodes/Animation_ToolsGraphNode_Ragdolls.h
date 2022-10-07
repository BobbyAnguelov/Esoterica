#pragma once
#include "Animation_ToolsGraphNode_DataSlot.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class PoweredRagdollToolsNode final : public DataSlotToolsNode
    {
        EE_REGISTER_TYPE( PoweredRagdollToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Powered Ragdoll"; }
        virtual char const* GetCategory() const override { return "Physics"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

        virtual char const* const GetDefaultSlotName() const override { return "Ragdoll"; }
        virtual ResourceTypeID GetSlotResourceTypeID() const override;

    private:

        EE_EXPOSE StringID                m_profileID;
        EE_EXPOSE float                   m_physicsBlendWeight = 1.0f;
        EE_EXPOSE bool                    m_isGravityEnabled = false;
    };

    //-------------------------------------------------------------------------

    class SimulatedRagdollToolsNode final : public DataSlotToolsNode
    {
        EE_REGISTER_TYPE( SimulatedRagdollToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Simulated Ragdoll"; }
        virtual char const* GetCategory() const override { return "Physics"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

        virtual char const* const GetDefaultSlotName() const override { return "Ragdoll"; }
        virtual ResourceTypeID GetSlotResourceTypeID() const override;

    private:

        virtual bool SupportsDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override { return "Exit Option"; }
        virtual uint32_t GetDynamicInputPinValueType() const override { return (uint32_t) GraphValueType::Pose; }
        virtual bool IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const override;

    private:

        // The profile to use when initializing the ragdoll simulation
        EE_EXPOSE StringID                m_entryProfileID;

        // The profile to use when "fully simulated"
        EE_EXPOSE StringID                m_simulatedProfileID;

        // The profile to use when leaving "fully simulated"
        EE_EXPOSE StringID                m_exitProfileID;
    };
}