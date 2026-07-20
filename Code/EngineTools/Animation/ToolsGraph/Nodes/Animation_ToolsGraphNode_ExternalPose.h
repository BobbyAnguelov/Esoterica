#pragma once
#include "Animation_ToolsGraphNode.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class ExternalPoseToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ExternalPoseToolsNode );

    public:

        ExternalPoseToolsNode();

        virtual char const *GetTypeName() const override { return "External Pose"; }
        virtual char const *GetCategory() const override { return "Animation"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext &context ) const override;

    private:

        virtual bool IsRenameable() const override { return true; }
        virtual bool RequiresUniqueName() const override { return true; }
        virtual String CreateUniqueNodeName( String const& desiredName ) const override final;
    };

    //-------------------------------------------------------------------------

    class IsExternalPoseSetToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( IsExternalPoseSetToolsNode );

    public:

        IsExternalPoseSetToolsNode();

        virtual char const *GetTypeName() const override { return "Is External Pose Filled"; }
        virtual char const *GetCategory() const override { return "Conditions"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::TransitionConduit, GraphType::BlendTree, GraphType::ValueTree, GraphType::EntryOverrideTree, GraphType::GlobalTransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext &context ) const override;

    private:

        EE_REFLECT();
        StringID m_slotID;
    };
}