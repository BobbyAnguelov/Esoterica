#pragma once
#include "Animation_ToolsGraphNode.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class ResultToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ResultToolsNode );

    public:

        ResultToolsNode() = default;
        ResultToolsNode( GraphValueType valueType );

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual GraphValueType GetValueType() const override { return GraphValueType::Unknown; }
        virtual char const* GetTypeName() const override { return "Result"; }
        virtual char const* GetCategory() const override { return "Results"; }
        virtual bool IsUserCreatable() const override { return false; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_REFLECT( "IsToolsReadOnly" : true );
        GraphValueType m_valueType = GraphValueType::Pose;
    };
}