#pragma once

#include "Animation_ToolsGraphNode.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class TwoBoneIKToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( TwoBoneIKToolsNode );

    public:

        TwoBoneIKToolsNode();

        virtual char const* GetTypeName() const override { return "Two Bone IK"; }
        virtual char const* GetCategory() const override { return "Animation/IK"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_REFLECT();
        StringID            m_effectorBoneID;

        EE_REFLECT();
        Percentage          m_allowedStretchPercentage = 0.0f;

        EE_REFLECT();
        bool                m_isTargetInWorldSpace = false;
    };
}