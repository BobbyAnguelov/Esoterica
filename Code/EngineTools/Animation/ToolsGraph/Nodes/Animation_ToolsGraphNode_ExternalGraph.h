#pragma once
#include "Animation_ToolsGraphNode.h"
#include "Engine/Animation/AnimationClip.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class ExternalGraphToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ExternalGraphToolsNode );

    public:

        ExternalGraphToolsNode();

        virtual bool IsRenameable() const override { return true; }
        virtual bool RequiresUniqueName() const override final { return true; }

        virtual char const* GetTypeName() const override { return "External Graph"; }
        virtual char const* GetCategory() const override { return "Animation/Graphs"; }
        virtual bool IsPersistentNode() const override { return true; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        virtual void DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;

        virtual String CreateUniqueNodeName( String const& desiredName ) const override final;
    };
}