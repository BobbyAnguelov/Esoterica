#pragma once

#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_VariationData.h"
#include "Engine/Animation/IK/IKRig.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class IKRigToolsNode final : public VariationDataToolsNode
    {
        EE_REFLECT_TYPE( IKRigToolsNode );

    public:

        struct Data final : public VariationDataToolsNode::Data
        {
            EE_REFLECT_TYPE( Data );

            virtual void GetReferencedResources( TInlineVector<ResourceID, 2>& outReferencedResources ) const override { outReferencedResources.emplace_back( m_rigDefinition.GetResourceID() ); }

        public:

            EE_REFLECT();
            TResourcePtr<IKRigDefinition>     m_rigDefinition;
        };

    public:

        IKRigToolsNode();

        virtual char const* GetTypeName() const override { return "IK Rig Solve"; }
        virtual char const* GetCategory() const override { return "Animation/IK"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

        virtual bool SupportsUserEditableDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override;
        virtual StringID GetDynamicInputPinValueType() const override { return GetPinTypeForValueType( GraphValueType::Target ); }
        virtual void PostDynamicPinDestruction() override;

    private:

        virtual TypeSystem::TypeInfo const* GetVariationDataTypeInfo() const override { return IKRigToolsNode::Data::s_pTypeInfo; }
    };
}