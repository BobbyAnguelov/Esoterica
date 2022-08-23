#pragma once
#include "Animation_ToolsGraphNode.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_IDs.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class IDComparisonToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( IDComparisonToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "ID Comparison"; }
        virtual char const* GetCategory() const override { return "Values/ID"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

        virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) override;

    private:

        EE_EXPOSE IDComparisonNode::Comparison     m_comparison = IDComparisonNode::Comparison::Matches;
        EE_EXPOSE TVector<StringID>                m_IDs;
    };

    //-------------------------------------------------------------------------

    class IDToFloatToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( IDToFloatToolsNode );

        struct Mapping : public IRegisteredType
        {
            EE_REGISTER_TYPE( Mapping );

            StringID    m_ID;
            float       m_value;
        };

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "ID to Float"; }
        virtual char const* GetCategory() const override { return "Values/ID"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_EXPOSE float                            m_defaultValue = 0.0f;
        EE_EXPOSE TVector<Mapping>                 m_mappings;
    };
}