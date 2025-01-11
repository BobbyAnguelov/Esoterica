#pragma once
#include "Animation_ToolsGraphNode_VariationData.h"
#include "Engine/Animation/AnimationClip.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class ZeroPoseToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ZeroPoseToolsNode );

    public:

        ZeroPoseToolsNode();

        virtual char const* GetTypeName() const override { return "Zero Pose"; }
        virtual char const* GetCategory() const override { return "Animation/Poses"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class ReferencePoseToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ReferencePoseToolsNode );

    public:

        ReferencePoseToolsNode();

        virtual char const* GetTypeName() const override { return "Reference Pose"; }
        virtual char const* GetCategory() const override { return "Animation/Poses"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class AnimationPoseToolsNode final : public VariationDataToolsNode
    {
        EE_REFLECT_TYPE( AnimationPoseToolsNode );

        struct Data final : public VariationDataToolsNode::Data
        {
            EE_REFLECT_TYPE( Data );

            virtual void GetReferencedResources( TInlineVector<ResourceID, 2>& outReferencedResources ) const override { outReferencedResources.emplace_back( m_animClip.GetResourceID() ); }

        public:

            EE_REFLECT();
            TResourcePtr<AnimationClip>     m_animClip;
        };

    public:

        AnimationPoseToolsNode();

        virtual char const* GetTypeName() const override { return "Animation Pose"; }
        virtual char const* GetCategory() const override { return "Animation/Poses"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

        virtual void DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;

    private:

        virtual TypeSystem::TypeInfo const* GetVariationDataTypeInfo() const override { return AnimationPoseToolsNode::Data::s_pTypeInfo; }

    private:

        // Use this to remap an input value into a valid 0~1 range.
        EE_REFLECT();
        FloatRange                          m_inputTimeRemapRange;

        // User specified fixed time
        EE_REFLECT();
        float                               m_fixedTimeValue = 0.0f;

        // Whether the input is in frames or percentage
        EE_REFLECT();
        bool                                m_useFramesAsInput = false;
    };
}