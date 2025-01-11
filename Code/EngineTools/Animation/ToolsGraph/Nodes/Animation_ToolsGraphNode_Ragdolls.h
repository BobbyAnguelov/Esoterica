#pragma once
#include "Animation_ToolsGraphNode_VariationData.h"
#include "Engine/Physics/PhysicsRagdoll.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class PoweredRagdollToolsNode final : public VariationDataToolsNode
    {
        EE_REFLECT_TYPE( PoweredRagdollToolsNode );

        struct Data final : public VariationDataToolsNode::Data
        {
            EE_REFLECT_TYPE( Data );

            virtual void GetReferencedResources( TInlineVector<ResourceID, 2>& outReferencedResources ) const override { outReferencedResources.emplace_back( m_ragdollDefinition.GetResourceID() ); }

        public:

            EE_REFLECT();
            TResourcePtr<Physics::RagdollDefinition>     m_ragdollDefinition;
        };

    public:

        PoweredRagdollToolsNode();

        virtual char const* GetTypeName() const override { return "Powered Ragdoll"; }
        virtual char const* GetCategory() const override { return "Animation/Physics"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        virtual TypeSystem::TypeInfo const* GetVariationDataTypeInfo() const override { return PoweredRagdollToolsNode::Data::s_pTypeInfo; }

    private:

        EE_REFLECT() StringID                m_profileID;
        EE_REFLECT() float                   m_physicsBlendWeight = 1.0f;
        EE_REFLECT() bool                    m_isGravityEnabled = false;
    };

    //-------------------------------------------------------------------------

    class SimulatedRagdollToolsNode final : public VariationDataToolsNode
    {
        EE_REFLECT_TYPE( SimulatedRagdollToolsNode );

        struct Data final : public VariationDataToolsNode::Data
        {
            EE_REFLECT_TYPE( Data );

            virtual void GetReferencedResources( TInlineVector<ResourceID, 2>& outReferencedResources ) const override { outReferencedResources.emplace_back( m_ragdollDefinition.GetResourceID() ); }

        public:

            EE_REFLECT();
            TResourcePtr<Physics::RagdollDefinition>     m_ragdollDefinition;
        };

    public:

        SimulatedRagdollToolsNode();

        virtual char const* GetTypeName() const override { return "Simulated Ragdoll"; }
        virtual char const* GetCategory() const override { return "Animation/Physics"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        virtual bool SupportsUserEditableDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override { return "Exit Option"; }
        virtual StringID GetDynamicInputPinValueType() const override { return GetPinTypeForValueType( GraphValueType::Pose ); }
        virtual bool IsValidConnection( UUID const& inputPinID, FlowNode const* pOutputPinNode, UUID const& outputPinID ) const override;

    private:

        virtual TypeSystem::TypeInfo const* GetVariationDataTypeInfo() const override { return SimulatedRagdollToolsNode::Data::s_pTypeInfo; }

    private:

        // The profile to use when initializing the ragdoll simulation
        EE_REFLECT() StringID                m_entryProfileID;

        // The profile to use when "fully simulated"
        EE_REFLECT() StringID                m_simulatedProfileID;

        // The profile to use when leaving "fully simulated"
        EE_REFLECT() StringID                m_exitProfileID;
    };
}