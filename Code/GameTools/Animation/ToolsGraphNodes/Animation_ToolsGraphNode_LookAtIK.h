#pragma once

#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class LookAtIKToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( LookAtIKToolsNode );

    public:

        LookAtIKToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Pose; }
        virtual char const* GetTypeName() const override { return "Look At"; }
        virtual char const* GetCategory() const override { return "IK"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };
}