#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"
#include "Engine/Animation/AnimationBoneMask.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    //-------------------------------------------------------------------------
    // Control Parameters
    //-------------------------------------------------------------------------

    class EE_ENGINE_API ControlParameterBoolNode final : public BoolValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public BoolValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;
        };

    private:

        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;
        virtual void SetValueInternal( void const* pInValue ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        bool m_value = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API ControlParameterIDNode final : public IDValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public IDValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;
        };

    private:

        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;
        virtual void SetValueInternal( void const* pInValue ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        StringID m_value;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API ControlParameterIntNode final : public IntValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public IntValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;
        };

    private:

        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;
        virtual void SetValueInternal( void const* pInValue ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        int32_t m_value = 0;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API ControlParameterFloatNode final : public FloatValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public FloatValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;
        };

    private:

        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;
        virtual void SetValueInternal( void const* pInValue ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        float m_value = 0.0f;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API ControlParameterVectorNode final : public VectorValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public VectorValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;
        };

    private:

        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;
        virtual void SetValueInternal( void const* pInValue ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        Vector m_value = Vector::Zero;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API ControlParameterTargetNode final : public TargetValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public TargetValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;
        };

    private:

        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;
        virtual void SetValueInternal( void const* pInValue ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        Target m_value;
    };

    //-------------------------------------------------------------------------
    // Virtual control parameters
    //-------------------------------------------------------------------------
    // NOTE: Virtual parameters are not allowed to cache values, as their result can change 

    class EE_ENGINE_API VirtualParameterBoolNode final : public BoolValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public BoolValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( BoolValueNode::Settings, m_childNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t m_childNodeIdx = InvalidIndex;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        BoolValueNode* m_pChildNode = nullptr;
        bool           m_value = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API VirtualParameterIDNode final : public IDValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public IDValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( IDValueNode::Settings, m_childNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t m_childNodeIdx = InvalidIndex;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        IDValueNode*    m_pChildNode = nullptr;
        StringID        m_value;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API VirtualParameterIntNode final : public IntValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public IntValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( IntValueNode::Settings, m_childNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t m_childNodeIdx = InvalidIndex;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        IntValueNode*   m_pChildNode = nullptr;
        int32_t         m_value = 0;
    };

    //-------------------------------------------------------------------------
     
    class EE_ENGINE_API VirtualParameterFloatNode final : public FloatValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public FloatValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( FloatValueNode::Settings, m_childNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t m_childNodeIdx = InvalidIndex;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        FloatValueNode* m_pChildNode = nullptr;
        float           m_value = 0.0f;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API VirtualParameterVectorNode final : public VectorValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public VectorValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( VectorValueNode::Settings, m_childNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t m_childNodeIdx = InvalidIndex;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        VectorValueNode*    m_pChildNode = nullptr;
        Float4              m_value = Float4::Zero;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API VirtualParameterTargetNode final : public TargetValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public TargetValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( TargetValueNode::Settings, m_childNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t m_childNodeIdx = InvalidIndex;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        TargetValueNode*    m_pChildNode = nullptr;
        Target              m_value;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API VirtualParameterBoneMaskNode final : public BoneMaskValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public BoneMaskValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( BoneMaskValueNode::Settings, m_childNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t m_childNodeIdx = InvalidIndex;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        BoneMaskValueNode*  m_pChildNode = nullptr;
        BoneMaskTaskList    m_value;
    };
}