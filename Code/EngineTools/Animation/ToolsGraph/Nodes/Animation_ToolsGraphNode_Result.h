#pragma once
#include "Animation_ToolsGraphNode.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class ResultToolsNode : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ResultToolsNode );

    public:

        ResultToolsNode();

        virtual char const* GetTypeName() const override { return "Result"; }
        virtual char const* GetCategory() const override { return "Results"; }
        virtual bool IsUserCreatable() const override { return false; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    protected:

        ResultToolsNode( GraphValueType valueType );

    private:

        EE_REFLECT( ReadOnly );
        GraphValueType m_valueType = GraphValueType::Special;
    };

    //-------------------------------------------------------------------------

    class PoseResultToolsNode final : public ResultToolsNode
    {
        EE_REFLECT_TYPE( PoseResultToolsNode );

    public:

        PoseResultToolsNode() : ResultToolsNode( GraphValueType::Pose ) {}
    };

    //-------------------------------------------------------------------------

    class BoolResultToolsNode final : public ResultToolsNode
    {
        EE_REFLECT_TYPE( BoolResultToolsNode );

    public:

        BoolResultToolsNode() : ResultToolsNode( GraphValueType::Bool ) {}
    };

    //-------------------------------------------------------------------------

    class IDResultToolsNode final : public ResultToolsNode
    {
        EE_REFLECT_TYPE( IDResultToolsNode );

    public:

        IDResultToolsNode() : ResultToolsNode( GraphValueType::ID ) {}
    };

    //-------------------------------------------------------------------------

    class FloatResultToolsNode final : public ResultToolsNode
    {
        EE_REFLECT_TYPE( FloatResultToolsNode );

    public:

        FloatResultToolsNode() : ResultToolsNode( GraphValueType::Float ) {}
    };

    //-------------------------------------------------------------------------

    class VectorResultToolsNode final : public ResultToolsNode
    {
        EE_REFLECT_TYPE( VectorResultToolsNode );

    public:

        VectorResultToolsNode() : ResultToolsNode( GraphValueType::Vector ) {}
    };

    //-------------------------------------------------------------------------

    class TargetResultToolsNode final : public ResultToolsNode
    {
        EE_REFLECT_TYPE( TargetResultToolsNode );

    public:

        TargetResultToolsNode() : ResultToolsNode( GraphValueType::Target ) {}
    };

    //-------------------------------------------------------------------------

    class BoneMaskResultToolsNode final : public ResultToolsNode
    {
        EE_REFLECT_TYPE( BoneMaskResultToolsNode );

    public:

        BoneMaskResultToolsNode() : ResultToolsNode( GraphValueType::BoneMask ) {}
    };
}