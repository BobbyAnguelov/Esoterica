#pragma once
#include "Animation_ToolsGraphNode.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Vectors.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class VectorInfoToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( VectorInfoToolsNode );

    public:

        VectorInfoToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
        virtual char const* GetTypeName() const override { return "Vector Info"; }
        virtual char const* GetCategory() const override { return "Values/Vector"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_REFLECT() VectorInfoNode::Info            m_desiredInfo = VectorInfoNode::Info::X;
    };

    //-------------------------------------------------------------------------

    class VectorCreateToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( VectorCreateToolsNode );

    public:

        VectorCreateToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Vector; }
        virtual char const* GetTypeName() const override { return "Vector Create"; }
        virtual char const* GetCategory() const override { return "Values/Vector"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class VectorNegateToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( VectorNegateToolsNode );

    public:

        VectorNegateToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Vector; }
        virtual char const* GetTypeName() const override { return "Vector Negate"; }
        virtual char const* GetCategory() const override { return "Values/Vector"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };
}