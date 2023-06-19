#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class EE_ENGINE_API ConstBoolNode final : public BoolValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public BoolValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( BoolValueNode::Settings, m_value );

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

        struct EE_ENGINE_API Settings final : public IDValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( IDValueNode::Settings, m_value );

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

        struct EE_ENGINE_API Settings final : public FloatValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( FloatValueNode::Settings, m_value );

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

        struct EE_ENGINE_API Settings final : public VectorValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( VectorValueNode::Settings, m_value );

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

        struct EE_ENGINE_API Settings final : public TargetValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( TargetValueNode::Settings, m_value );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            Target m_value;
        };

    private:

        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;
    };
}