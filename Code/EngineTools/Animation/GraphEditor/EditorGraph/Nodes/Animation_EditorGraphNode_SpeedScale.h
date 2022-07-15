#pragma once

#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_FlowGraph.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class SpeedScaleEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( SpeedScaleEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Speed Scale"; }
        virtual char const* GetCategory() const override { return "Utility"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_EXPOSE FloatRange              m_scaleLimits = FloatRange( 0.01f, 10.0f );
        EE_EXPOSE float                   m_blendTime = 0.2f;
    };

    //-------------------------------------------------------------------------

    class VelocityBasedSpeedScaleEditorNode final : public EditorGraphNode
    {
        EE_REGISTER_TYPE( VelocityBasedSpeedScaleEditorNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "Velocity Based Speed Scale"; }
        virtual char const* GetCategory() const override { return "Utility"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_EXPOSE float                   m_blendTime = 0.2f;
    };
}