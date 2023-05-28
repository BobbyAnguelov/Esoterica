#pragma once
#include "Engine/Animation/AnimationClip.h"
#include "Animation_ToolsGraphNode_DataSlot.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class AnimationClipToolsNode final : public DataSlotToolsNode
    {
        EE_REFLECT_TYPE( AnimationClipToolsNode );

    public:

        AnimationClipToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Pose; }
        virtual char const* GetTypeName() const override { return "Animation Clip"; }
        virtual char const* GetCategory() const override { return "Animation"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

        virtual char const* GetDefaultSlotName() const override { return "Animation"; }
        virtual ResourceTypeID GetSlotResourceTypeID() const override { return AnimationClip::GetStaticResourceTypeID(); }

    private:

        EE_REFLECT() bool         m_sampleRootMotion = true;
        EE_REFLECT() bool         m_allowLooping = false;
    };

    //-------------------------------------------------------------------------

    // This is a base class for all nodes that directly reference an animation clip
    class AnimationClipReferenceToolsNode : public FlowToolsNode
    {
        EE_REFLECT_TYPE( AnimationClipReferenceToolsNode );
    };
}