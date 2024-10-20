#pragma once
#include "Animation_ToolsGraphNode.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_IDs.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class IDComparisonToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( IDComparisonToolsNode );

    public:

        IDComparisonToolsNode();

        virtual char const* GetTypeName() const override { return "ID Comparison"; }
        virtual char const* GetCategory() const override { return "Values/ID"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

        virtual void DrawInfoText( NodeGraph::DrawContext const& ctx ) override;
        virtual void GetLogicAndEventIDs( TVector<StringID>& outIDs ) const override;
        virtual void RenameLogicAndEventIDs( StringID oldID, StringID newID ) override;

    private:

        EE_REFLECT();
        IDComparisonNode::Comparison     m_comparison = IDComparisonNode::Comparison::Matches;

        EE_REFLECT( CustomEditor = "AnimGraph_ID" );
        TVector<StringID>                m_IDs;
    };

    //-------------------------------------------------------------------------

    class IDToFloatToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( IDToFloatToolsNode );

        struct Mapping : public IReflectedType
        {
            EE_REFLECT_TYPE( Mapping );

            EE_REFLECT( CustomEditor = "AnimGraph_ID" );
            StringID    m_ID;

            EE_REFLECT();
            float       m_value;
        };

    public:

        IDToFloatToolsNode();

        virtual char const* GetTypeName() const override { return "ID to Float"; }
        virtual char const* GetCategory() const override { return "Values/ID"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void DrawInfoText( NodeGraph::DrawContext const& ctx ) override;
        virtual void GetLogicAndEventIDs( TVector<StringID>& outIDs ) const override;
        virtual void RenameLogicAndEventIDs( StringID oldID, StringID newID ) override;

    private:

        bool ValidateMappings() const;

    private:

        EE_REFLECT();
        float                            m_defaultValue = 0.0f;

        EE_REFLECT();
        TVector<Mapping>                 m_mappings;
    };
}