#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    enum class CachedValueMode
    {
        EE_REGISTER_ENUM

        OnEntry = 0,
        OnExit
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API CachedBoolNode final : public BoolValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public BoolValueNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( BoolValueNode::Settings, m_inputValueNodeIdx, m_mode );

            virtual void InstantiateNode( TVector<GraphNode*> const& nodePtrs, GraphDataSet const* pDataSet, InstantiationOptions options ) const override;

            int16_t      m_inputValueNodeIdx = InvalidIndex;
            CachedValueMode     m_mode;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

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
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( IDValueNode::Settings, m_inputValueNodeIdx, m_mode );

            virtual void InstantiateNode( TVector<GraphNode*> const& nodePtrs, GraphDataSet const* pDataSet, InstantiationOptions options ) const override;

            int16_t      m_inputValueNodeIdx = InvalidIndex;
            CachedValueMode     m_mode;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

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
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( IntValueNode::Settings, m_inputValueNodeIdx, m_mode );

            virtual void InstantiateNode( TVector<GraphNode*> const& nodePtrs, GraphDataSet const* pDataSet, InstantiationOptions options ) const override;

            int16_t      m_inputValueNodeIdx = InvalidIndex;
            CachedValueMode     m_mode;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        IntValueNode*               m_pInputValueNode = nullptr;
        int32_t                       m_value;
        bool                        m_hasCachedValue = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API CachedFloatNode final : public FloatValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public FloatValueNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( FloatValueNode::Settings, m_inputValueNodeIdx, m_mode );

            virtual void InstantiateNode( TVector<GraphNode*> const& nodePtrs, GraphDataSet const* pDataSet, InstantiationOptions options ) const override;

            int16_t      m_inputValueNodeIdx = InvalidIndex;
            CachedValueMode     m_mode;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        FloatValueNode*             m_pInputValueNode = nullptr;
        float                       m_previousValue;
        float                       m_currentValue;
        bool                        m_hasCachedValue = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API CachedVectorNode final : public VectorValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public VectorValueNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( VectorValueNode::Settings, m_inputValueNodeIdx, m_mode );

            virtual void InstantiateNode( TVector<GraphNode*> const& nodePtrs, GraphDataSet const* pDataSet, InstantiationOptions options ) const override;

            int16_t      m_inputValueNodeIdx = InvalidIndex;
            CachedValueMode     m_mode;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

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
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( TargetValueNode::Settings, m_inputValueNodeIdx, m_mode );

            virtual void InstantiateNode( TVector<GraphNode*> const& nodePtrs, GraphDataSet const* pDataSet, InstantiationOptions options ) const override;

            int16_t      m_inputValueNodeIdx = InvalidIndex;
            CachedValueMode     m_mode;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        TargetValueNode*            m_pInputValueNode = nullptr;
        Target                      m_value;
        bool                        m_hasCachedValue = false;
    };
}