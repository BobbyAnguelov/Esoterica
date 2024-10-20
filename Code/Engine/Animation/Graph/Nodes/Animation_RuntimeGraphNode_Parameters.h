#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"
#include "Engine/Animation/AnimationBoneMask.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphInstance;
}

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    //-------------------------------------------------------------------------
    // Control Parameters
    //-------------------------------------------------------------------------

    class EE_ENGINE_API ControlParameterBoolNode final : public BoolValueNode
    {
        friend GraphInstance;

    public:

        struct EE_ENGINE_API Definition final : public BoolValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;
        };

    private:

        EE_FORCE_INLINE void DirectlySetValue( bool value ) { m_value = value; }
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;
        virtual void SetValueInternal( void const* pInValue ) override { m_value = *(bool*) pInValue; }

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
        friend GraphInstance;

    public:

        struct EE_ENGINE_API Definition final : public IDValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;
        };

    private:

        EE_FORCE_INLINE void DirectlySetValue( StringID value ) { m_value = value; }
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;
        virtual void SetValueInternal( void const* pInValue ) override { m_value = *(StringID*) pInValue; }

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        StringID m_value;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API ControlParameterFloatNode final : public FloatValueNode
    {
        friend GraphInstance;

    public:

        struct EE_ENGINE_API Definition final : public FloatValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;
        };

    private:

        EE_FORCE_INLINE void DirectlySetValue( float value ) { m_value = value; }
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;
        virtual void SetValueInternal( void const* pInValue ) override { m_value = *(float*) pInValue; }

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
        friend GraphInstance;

    public:

        struct EE_ENGINE_API Definition final : public VectorValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;
        };

    private:

        EE_FORCE_INLINE void DirectlySetValue( Vector const& value ) { m_value = value; }
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;
        virtual void SetValueInternal( void const* pInValue ) override { m_value = *(Float3*) pInValue; }

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        Float3 m_value = Float3::Zero;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API ControlParameterTargetNode final : public TargetValueNode
    {
        friend GraphInstance;

    public:

        struct EE_ENGINE_API Definition final : public TargetValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;
        };

    private:

        EE_FORCE_INLINE void DirectlySetValue( Target const& value ) { m_value = value; }
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;
        virtual void SetValueInternal( void const* pInValue ) override { m_value = *(Target*) pInValue; }

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

        struct EE_ENGINE_API Definition final : public BoolValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( BoolValueNode::Definition, m_childNodeIdx );

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

        struct EE_ENGINE_API Definition final : public IDValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( IDValueNode::Definition, m_childNodeIdx );

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
     
    class EE_ENGINE_API VirtualParameterFloatNode final : public FloatValueNode
    {
    public:

        struct EE_ENGINE_API Definition final : public FloatValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( FloatValueNode::Definition, m_childNodeIdx );

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

        struct EE_ENGINE_API Definition final : public VectorValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( VectorValueNode::Definition, m_childNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t m_childNodeIdx = InvalidIndex;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        VectorValueNode*    m_pChildNode = nullptr;
        Float3              m_value = Float3::Zero;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API VirtualParameterTargetNode final : public TargetValueNode
    {
    public:

        struct EE_ENGINE_API Definition final : public TargetValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( TargetValueNode::Definition, m_childNodeIdx );

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

        struct EE_ENGINE_API Definition final : public BoneMaskValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( BoneMaskValueNode::Definition, m_childNodeIdx );

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