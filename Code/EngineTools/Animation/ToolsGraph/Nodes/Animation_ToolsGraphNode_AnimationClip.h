#pragma once
#include "Engine/Animation/AnimationClip.h"
#include "Animation_ToolsGraphNode_VariationData.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class AnimationClipToolsNode final : public VariationDataToolsNode
    {
        EE_REFLECT_TYPE( AnimationClipToolsNode );

    public:

        struct Data final : public VariationDataToolsNode::Data
        {
            EE_REFLECT_TYPE( Data );

            virtual void GetReferencedResources( TInlineVector<ResourceID, 2>& outReferencedResources ) const override { outReferencedResources.emplace_back( m_animClip.GetResourceID() ); }

        public:

            EE_REFLECT();
            TResourcePtr<AnimationClip>                 m_animClip;

            EE_REFLECT( Min = "0.01", Max = "5.0f" );
            float                                       m_speedMultiplier = 1.0f;

            EE_REFLECT();
            int32_t                                     m_startSyncEventOffset = 0;
        };

    public:

        AnimationClipToolsNode();

        virtual bool IsAnimationClipReferenceNode() const override { return true; }
        virtual char const* GetTypeName() const override { return "Animation Clip"; }
        virtual char const* GetCategory() const override { return "Animation"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        virtual TypeSystem::TypeInfo const* GetVariationDataTypeInfo() const override { return AnimationClipToolsNode::Data::s_pTypeInfo; }

    private:

        EE_REFLECT() bool                               m_sampleRootMotion = true;
        EE_REFLECT() bool                               m_allowLooping = false;
    };
}