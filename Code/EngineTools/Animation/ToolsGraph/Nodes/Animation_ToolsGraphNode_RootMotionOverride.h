#pragma once
#include "Animation_ToolsGraphNode.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class RootMotionOverrideToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( RootMotionOverrideToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Pose; }
        virtual char const* GetTypeName() const override { return "Root Motion Override"; }
        virtual char const* GetCategory() const override { return "Animation/Root Motion"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        // Set to negative to disable the velocity limiter
        EE_EXPOSE float                m_maxLinearVelocity = -1.0f;

        // Set to negative to disable the velocity limiter
        EE_EXPOSE float                m_maxAngularVelocity = -1.0f;

        // Limits
        EE_EXPOSE bool                 m_overrideHeadingX = true;
        EE_EXPOSE bool                 m_overrideHeadingY = true;
        EE_EXPOSE bool                 m_overrideHeadingZ = true;
        EE_EXPOSE bool                 m_allowPitchForFacing = false;
    };
}