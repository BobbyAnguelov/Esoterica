#pragma once

#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_DataSlot.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class IKRigToolsNode final : public DataSlotToolsNode
    {
        EE_REFLECT_TYPE( IKRigToolsNode );

    public:

        IKRigToolsNode();

        virtual char const* GetTypeName() const override { return "IK Rig Solve"; }
        virtual char const* GetCategory() const override { return "Animation/IK"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

        virtual char const* GetDefaultSlotName() const override { return "IK Rig"; }
        virtual ResourceTypeID GetSlotResourceTypeID() const override;

        virtual bool SupportsUserEditableDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override;
        virtual StringID GetDynamicInputPinValueType() const override { return GetPinTypeForValueType( GraphValueType::Target ); }
        virtual void PostDynamicPinDestruction() override;
    };
}