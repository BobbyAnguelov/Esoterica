#pragma once
#include "Animation_ToolsGraphNode.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class ExternalReferencedGraphToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ExternalReferencedGraphToolsNode );

    public:

        ExternalReferencedGraphToolsNode();

        virtual bool IsRenameable() const override { return true; }
        virtual bool RequiresUniqueName() const override final { return true; }

        virtual char const* GetTypeName() const override { return "External Graph"; }
        virtual char const* GetCategory() const override { return "Animation/Graphs"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        virtual void DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;

        virtual String CreateUniqueNodeName( String const& desiredName ) const override final;
    };

    //-------------------------------------------------------------------------

    class IsExternalGraphSlotFilledToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( IsExternalGraphSlotFilledToolsNode );

    public:

        IsExternalGraphSlotFilledToolsNode();

        virtual char const *GetTypeName() const override { return "Is External Slot Filled"; }
        virtual char const *GetCategory() const override { return "Conditions"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionConduit, GraphType::BlendTree, GraphType::ValueTree, GraphType::EntryOverrideTree, GraphType::GlobalTransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext &context ) const override;

    private:

        EE_REFLECT();
        StringID m_slotID;
    };
}