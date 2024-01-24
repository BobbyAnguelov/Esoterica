#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class EE_ENGINE_API AndNode final : public BoolValueNode
    {
    public:

        struct EE_ENGINE_API Definition final : public BoolValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( BoolValueNode::Definition, m_conditionNodeIndices );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            TInlineVector<int16_t, 4>            m_conditionNodeIndices;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        TInlineVector<BoolValueNode*, 4>            m_conditionNodes;
        bool                                        m_result = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API OrNode final : public BoolValueNode
    {
    public:

        struct EE_ENGINE_API Definition final : public BoolValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( BoolValueNode::Definition, m_conditionNodeIndices );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            TInlineVector<int16_t, 4>            m_conditionNodeIndices;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        TInlineVector<BoolValueNode*, 4>            m_conditionNodes;
        bool                                        m_result = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API NotNode final : public BoolValueNode
    {
    public:

        struct EE_ENGINE_API Definition final : public BoolValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( BoolValueNode::Definition, m_inputValueNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                               m_inputValueNodeIdx = InvalidIndex;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        BoolValueNode*                              m_pInputValueNode = nullptr;
        bool                                        m_result = false;
    };
}