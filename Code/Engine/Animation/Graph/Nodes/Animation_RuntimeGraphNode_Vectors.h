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
            EE_REFLECT_ENUM

            X,
            Y,
            Z,
            W,
            Length,
            AngleHorizontal, // The horizontal angle between the vector and the character forward (assumes the vector is in character space)
            AngleVertical, // The vertical angle (around x) between the vector and the character forward (assumes the vector is in character space)
        };

    public:

        struct EE_ENGINE_API Settings final : public FloatValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( FloatValueNode::Settings, m_inputValueNodeIdx, m_desiredInfo );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                     m_inputValueNodeIdx = InvalidIndex;
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

    class EE_ENGINE_API VectorCreateNode final : public VectorValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public VectorValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( VectorValueNode::Settings, m_inputVectorValueNodeIdx, m_inputValueXNodeIdx, m_inputValueYNodeIdx, m_inputValueZNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                     m_inputVectorValueNodeIdx = InvalidIndex;
            int16_t                     m_inputValueXNodeIdx = InvalidIndex;
            int16_t                     m_inputValueYNodeIdx = InvalidIndex;
            int16_t                     m_inputValueZNodeIdx = InvalidIndex;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        VectorValueNode*                m_pInputVectorValueNode = nullptr;
        FloatValueNode*                 m_pInputXValueNode = nullptr;
        FloatValueNode*                 m_pInputYValueNode = nullptr;
        FloatValueNode*                 m_pInputZValueNode = nullptr;
        Vector                          m_value = Vector::Zero;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API VectorNegateNode final : public VectorValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public VectorValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( VectorValueNode::Settings, m_inputValueNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                     m_inputValueNodeIdx = InvalidIndex;
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