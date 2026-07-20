#pragma once

#include "Animation_ToolsGraphNode.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class ScaleToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ScaleToolsNode );

    public:

        ScaleToolsNode();

        virtual char const* GetTypeName() const override { return "Scale"; }
        virtual char const* GetCategory() const override { return "Animation"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };
}