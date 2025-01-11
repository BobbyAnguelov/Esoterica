#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API IsTargetSetNode final : public BoolValueNode
    {
    public:

        struct EE_ENGINE_API Definition final : public BoolValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( BoolValueNode::Definition, m_inputValueNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                     m_inputValueNodeIdx = InvalidIndex;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        TargetValueNode*                m_pInputValueNode = nullptr;
        bool                            m_result = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API TargetInfoNode final : public FloatValueNode
    {
    public:

        enum class Info
        {
            EE_REFLECT_ENUM

            AngleHorizontal, // The horizontal angle (around z) between the target point and the character
            AngleVertical, // The vertical angle (around x) between the target point and the character

            Distance, // The distance to the target point
            DistanceHorizontalOnly, // The XY distance to the target point
            DistanceVerticalOnly, // The Z distance to the target point

            DeltaOrientationX, // The difference in orientation between the character and the target orientation (assumes target is in worldspace)
            DeltaOrientationY, // The difference in orientation between the character and the target orientation (assumes target is in worldspace)
            DeltaOrientationZ, // The difference in orientation between the character and the target orientation (assumes target is in worldspace)
        };

        struct EE_ENGINE_API Definition final : public FloatValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( FloatValueNode::Definition, m_inputValueNodeIdx, m_isWorldSpaceTarget, m_infoType );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                     m_inputValueNodeIdx = InvalidIndex;
            Info                        m_infoType = Info::Distance;
            bool                        m_isWorldSpaceTarget = true;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        TargetValueNode*                m_pTargetNode = nullptr;
        float                           m_value = 0.0f;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API TargetPointNode final : public VectorValueNode
    {
    public:

        struct EE_ENGINE_API Definition final : public VectorValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( VectorValueNode::Definition, m_inputValueNodeIdx, m_isWorldSpaceTarget );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                     m_inputValueNodeIdx = InvalidIndex;
            bool                        m_isWorldSpaceTarget = true;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        TargetValueNode*                m_pTargetNode = nullptr;
        Vector                          m_value = Vector::Zero;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API TargetOffsetNode final : public TargetValueNode
    {

    public:

        struct EE_ENGINE_API Definition final : public TargetValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( TargetValueNode::Definition, m_inputValueNodeIdx, m_isBoneSpaceOffset, m_rotationOffset, m_translationOffset );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                     m_inputValueNodeIdx = InvalidIndex;
            bool                        m_isBoneSpaceOffset = true;
            Quaternion                  m_rotationOffset = Quaternion::Identity;
            Vector                      m_translationOffset = Vector::Zero;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        TargetValueNode*                m_pInputValueNode = nullptr;
        Target                          m_value;
    };
}