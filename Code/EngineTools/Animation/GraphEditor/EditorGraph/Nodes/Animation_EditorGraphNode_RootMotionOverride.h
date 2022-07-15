#pragma once
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_FlowGraph.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class RootMotionOverrideEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( RootMotionOverrideEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Root Motion Override"; }
        virtual char const* GetCategory() const override { return "Utility"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        // Set to negative to disable the velocity limiter
        EE_EXPOSE float                m_maxLinearVelocity = -1.0f;

        // Set to negative to disable the velocity limiter
        EE_EXPOSE float                m_maxAngularVelocity = -1.0f;

        EE_EXPOSE bool                 m_overrideHeadingX = true;
        EE_EXPOSE bool                 m_overrideHeadingY = true;
        EE_EXPOSE bool                 m_overrideHeadingZ = true;
        EE_EXPOSE bool                 m_overrideFacingX = true;
        EE_EXPOSE bool                 m_overrideFacingY = true;
        EE_EXPOSE bool                 m_overrideFacingZ = true;
    };
}