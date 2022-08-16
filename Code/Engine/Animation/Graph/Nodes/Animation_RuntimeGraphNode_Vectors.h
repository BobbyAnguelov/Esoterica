#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"
#include "Engine/Math/Easing.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class EE_ENGINE_API VectorInfoNode final : public FloatValueNode
    {
    public:

        enum class Info : uint8_t
        {
            EE_REGISTER_ENUM

            X,
            Y,
            Z,
            W,
            Length,
            AngleHorizontal,
            AngleVertical,
            SizeHorizontal,
            SizeVertical,
        };

    public:

        struct EE_ENGINE_API Settings final : public FloatValueNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( FloatValueNode::Settings, m_inputValueNodeIdx, m_desiredInfo );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                   m_inputValueNodeIdx = InvalidIndex;
            Info                        m_desiredInfo = Info::X;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        VectorValueNode*                m_pInputValueNode = nullptr;
        float                           m_value = 0.0f;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API VectorNegateNode final : public VectorValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public VectorValueNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( VectorValueNode::Settings, m_inputValueNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                   m_inputValueNodeIdx = InvalidIndex;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        VectorValueNode*                m_pInputValueNode = nullptr;
        Vector                          m_value = Vector::Zero;
    };
}