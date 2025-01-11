#pragma once

#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class AimIKToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( AimIKToolsNode );

    public:

        AimIKToolsNode();

        virtual char const* GetTypeName() const override { return "Aim At"; }
        virtual char const* GetCategory() const override { return "Animation/IK"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };
}