#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class EE_ENGINE_API IsTargetSetNode final : public BoolValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public BoolValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( BoolValueNode::Settings, m_inputValueNodeIdx );

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

        struct EE_ENGINE_API Settings final : public TargetValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( TargetValueNode::Settings, m_inputValueNodeIdx, m_isWorldSpaceTarget, m_infoType );

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

        VectorValueNode*                m_pTargetNode = nullptr;
        float                           m_value = 0.0f;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API TargetOffsetNode final : public TargetValueNode
    {

    public:

        struct EE_ENGINE_API Settings final : public TargetValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( TargetValueNode::Settings, m_inputValueNodeIdx, m_isBoneSpaceOffset, m_rotationOffset, m_translationOffset );

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