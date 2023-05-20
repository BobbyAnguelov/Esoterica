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

        struct EE_ENGINE_API Settings final : public BoolValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( BoolValueNode::Settings, m_inputValueNodeIdx, m_mode );

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

        struct EE_ENGINE_API Settings final : public IDValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( IDValueNode::Settings, m_inputValueNodeIdx, m_mode );

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

    class EE_ENGINE_API CachedIntNode final : public IntValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public IntValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( IntValueNode::Settings, m_inputValueNodeIdx, m_mode );

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

        IntValueNode*               m_pInputValueNode = nullptr;
        int32_t                     m_value;
        bool                        m_hasCachedValue = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API CachedFloatNode final : public FloatValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public FloatValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( FloatValueNode::Settings, m_inputValueNodeIdx, m_mode );

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

        struct EE_ENGINE_API Settings final : public VectorValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( VectorValueNode::Settings, m_inputValueNodeIdx, m_mode );

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

        struct EE_ENGINE_API Settings final : public TargetValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( TargetValueNode::Settings, m_inputValueNodeIdx, m_mode );

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