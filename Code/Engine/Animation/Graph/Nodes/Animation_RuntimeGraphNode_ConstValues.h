#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class EE_ENGINE_API ConstBoolNode final : public BoolValueNode
    {
    public:

        struct EE_ENGINE_API Definition final : public BoolValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( BoolValueNode::Definition, m_value );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            bool m_value;
        };

    private:

        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API ConstIDNode final : public IDValueNode
    {
    public:

        struct EE_ENGINE_API Definition final : public IDValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( IDValueNode::Definition, m_value );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            StringID m_value;
        };

    private:

        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API ConstFloatNode final : public FloatValueNode
    {
    public:

        struct EE_ENGINE_API Definition final : public FloatValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( FloatValueNode::Definition, m_value );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            float m_value;
        };

    private:

        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API ConstVectorNode final : public VectorValueNode
    {
    public:

        struct EE_ENGINE_API Definition final : public VectorValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( VectorValueNode::Definition, m_value );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            Vector m_value;
        };

    private:

        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API ConstTargetNode final : public TargetValueNode
    {
    public:

        struct EE_ENGINE_API Definition final : public TargetValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( TargetValueNode::Definition, m_value );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            Target m_value;
        };

    private:

        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;
    };
}