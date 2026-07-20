#pragma once

#include "Animation_ToolsGraphNode_VariationData.h"
#include "Engine/Animation/IK/IK.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class TwoBoneIKToolsNode final : public VariationDataToolsNode
    {
        EE_REFLECT_TYPE( TwoBoneIKToolsNode );

    public:

        struct Data final : public VariationDataToolsNode::Data
        {
            EE_REFLECT_TYPE( Data );

        public:

            // The effector bone ID
            EE_REFLECT();
            StringID            m_effectorBoneID;

            // The amount of time to blend IK in or out
            EE_REFLECT();
            Seconds             m_blendTime = 0.0f;
        };

    public:

        TwoBoneIKToolsNode();

        virtual char const* GetTypeName() const override { return "Two Bone IK"; }
        virtual char const* GetCategory() const override { return "Animation/IK"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        virtual TypeSystem::TypeInfo const* GetVariationDataTypeInfo() const override { return TwoBoneIKToolsNode::Data::s_pTypeInfo; }

        virtual void DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;

    private:

        EE_REFLECT();
        bool                m_isTargetInWorldSpace = false;

        EE_REFLECT();
        IKBlendMode         m_blendMode = IKBlendMode::Effector;

        EE_REFLECT();
        float               m_referencePoseTwistWeight = 0.0f;
    };
}