#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    enum class CachedValueMode
    {
        EE_REFLECT_ENUM

        OnEntry = 0,
        OnExit
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API CachedBoolNode final : public BoolValueNode
    {
    public:

        struct EE_ENGINE_API Definition final : public BoolValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( BoolValueNode::Definition, m_inputValueNodeIdx, m_mode );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                 m_inputValueNodeIdx = InvalidIndex;
            CachedValueMode         m_mode;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        BoolValueNode*              m_pInputValueNode = nullptr;
        bool                        m_value;
        bool                        m_hasCachedValue = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API CachedIDNode final : public IDValueNode
    {
    public:

        struct EE_ENGINE_API Definition final : public IDValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( IDValueNode::Definition, m_inputValueNodeIdx, m_mode );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                 m_inputValueNodeIdx = InvalidIndex;
            CachedValueMode         m_mode;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        IDValueNode*                m_pInputValueNode = nullptr;
        StringID                    m_value;
        bool                        m_hasCachedValue = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API CachedFloatNode final : public FloatValueNode
    {
    public:

        struct EE_ENGINE_API Definition final : public FloatValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( FloatValueNode::Definition, m_inputValueNodeIdx, m_mode );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                 m_inputValueNodeIdx = InvalidIndex;
            CachedValueMode         m_mode;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        FloatValueNode*             m_pInputValueNode = nullptr;
        float                       m_value;
        bool                        m_hasCachedValue = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API CachedVectorNode final : public VectorValueNode
    {
    public:

        struct EE_ENGINE_API Definition final : public VectorValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( VectorValueNode::Definition, m_inputValueNodeIdx, m_mode );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                 m_inputValueNodeIdx = InvalidIndex;
            CachedValueMode         m_mode;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        VectorValueNode*            m_pInputValueNode = nullptr;
        Vector                      m_value;
        bool                        m_hasCachedValue = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API CachedTargetNode final : public TargetValueNode
    {
    public:

        struct EE_ENGINE_API Definition final : public TargetValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( TargetValueNode::Definition, m_inputValueNodeIdx, m_mode );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                 m_inputValueNodeIdx = InvalidIndex;
            CachedValueMode         m_mode;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        TargetValueNode*            m_pInputValueNode = nullptr;
        Target                      m_value;
        bool                        m_hasCachedValue = false;
    };
}