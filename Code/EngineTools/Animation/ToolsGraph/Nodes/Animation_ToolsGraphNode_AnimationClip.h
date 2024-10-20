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

        virtual bool IsAnimationClipReferenceNode() const override { return true; }
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
}