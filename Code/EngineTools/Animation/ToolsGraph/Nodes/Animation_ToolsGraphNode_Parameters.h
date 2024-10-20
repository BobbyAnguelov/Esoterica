#pragma once
#include "Animation_ToolsGraphNode.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class FlowGraph;
    class ToolsGraphDefinition;
}

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class ParameterBaseToolsNode : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ParameterBaseToolsNode );

    public:

        ParameterBaseToolsNode() = default;
        ParameterBaseToolsNode( String const& name, String const& groupName = String() );

        virtual bool IsVisible() const override { return false; }
        virtual bool IsRenameable() const override final { return true; }
        virtual bool IsUserCreatable() const override { return false; }
        virtual bool RequiresUniqueName() const override final { return true; }

        inline String const& GetParameterName() const { return m_name; }
        inline StringID GetParameterID() const { return StringID( m_name ); }
        inline String const& GetParameterGroup() const { return m_group; }
        void SetParameterGroup( String const& newGroupName );

    private:

        EE_REFLECT( ReadOnly );
        String                     m_group;
    };

    //-------------------------------------------------------------------------
    // Control Parameter
    //-------------------------------------------------------------------------

    class ControlParameterToolsNode : public ParameterBaseToolsNode
    {
        EE_REFLECT_TYPE( ControlParameterToolsNode );

    public:

        static ControlParameterToolsNode* Create( FlowGraph* pRootGraph, GraphValueType type, String const& name, String const& groupName );

    public:

        using ParameterBaseToolsNode::ParameterBaseToolsNode;

        virtual char const* GetTypeName() const override { return "Parameter"; }
        virtual char const* GetCategory() const override { return "Control Parameters"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual bool IsPersistentNode() const override { return true; }

        virtual void ReflectPreviewValues( ControlParameterToolsNode const* pOtherParameterNode ) {}
    };

    //-------------------------------------------------------------------------

    class BoolControlParameterToolsNode : public ControlParameterToolsNode
    {
        EE_REFLECT_TYPE( BoolControlParameterToolsNode );

    public:

        BoolControlParameterToolsNode();
        BoolControlParameterToolsNode( String const& name, String const& groupName = String() );

        inline bool GetPreviewStartValue() const { return m_previewStartValue; }

    private:

        EE_REFLECT() bool m_previewStartValue = false;
    };

    //-------------------------------------------------------------------------

    class FloatControlParameterToolsNode : public ControlParameterToolsNode
    {
        EE_REFLECT_TYPE( FloatControlParameterToolsNode );

    public:

        FloatControlParameterToolsNode();
        FloatControlParameterToolsNode( String const& name, String const& groupName = String() );

        inline float GetPreviewStartValue() const { return m_previewStartValue; }
        inline float GetPreviewRangeMin() const { return m_previewMin; }
        inline float GetPreviewRangeMax() const { return m_previewMax; }

    private:

        EE_REFLECT() float m_previewStartValue = 0;
        EE_REFLECT() float m_previewMin = 0;
        EE_REFLECT() float m_previewMax = 1;
    };

    //-------------------------------------------------------------------------

    class IDControlParameterToolsNode : public ControlParameterToolsNode
    {
        EE_REFLECT_TYPE( IDControlParameterToolsNode );

    public:

        IDControlParameterToolsNode();
        IDControlParameterToolsNode( String const& name, String const& groupName = String() );

        inline StringID GetPreviewStartValue() const { return m_previewStartValue; }

        virtual void GetLogicAndEventIDs( TVector<StringID>& outIDs ) const override;
        virtual void RenameLogicAndEventIDs( StringID oldID, StringID newID ) override;
        virtual void ReflectPreviewValues( ControlParameterToolsNode const* pOtherParameterNode ) override;

        // Get all the expected preview values for this parameter
        TVector<StringID> const& GetExpectedPreviewValues() const { return m_expectedValues; }

    private:

        EE_REFLECT( CustomEditor = "AnimGraph_ID" );
        StringID                    m_previewStartValue;

        EE_REFLECT();
        TVector<StringID>           m_expectedValues;
    };

    //-------------------------------------------------------------------------

    class VectorControlParameterToolsNode : public ControlParameterToolsNode
    {
        EE_REFLECT_TYPE( VectorControlParameterToolsNode );

    public:

        VectorControlParameterToolsNode();
        VectorControlParameterToolsNode( String const& name, String const& groupName = String() );

        inline Float3 GetPreviewStartValue() const { return m_previewStartValue; }

    private:

        EE_REFLECT() Float3 m_previewStartValue = Float3::Zero;
    };

    //-------------------------------------------------------------------------

    class TargetControlParameterToolsNode : public ControlParameterToolsNode
    {
        EE_REFLECT_TYPE( TargetControlParameterToolsNode );

    public:

        TargetControlParameterToolsNode();
        TargetControlParameterToolsNode( String const& name, String const& groupName = String() );

        Target GetPreviewStartValue() const;
        inline bool IsSet() const { return m_isSet; }
        inline bool IsBoneTarget() const { return m_isBoneID; }
        inline Transform const& GetPreviewTargetTransform() const { return m_previewStartTransform; }
        void SetPreviewTargetTransform( Transform const& transform );

    private:

        EE_REFLECT();
        bool         m_isSet = false;

        EE_REFLECT( Category = "Bone" );
        bool         m_isBoneID = false;

        EE_REFLECT( Category = "Bone" );
        StringID     m_previewStartBoneID;

        EE_REFLECT( Category = "Transform" );
        Transform    m_previewStartTransform = Transform::Identity;
    };

    //-------------------------------------------------------------------------
    // Virtual Parameter
    //-------------------------------------------------------------------------

    class VirtualParameterToolsNode : public ParameterBaseToolsNode
    {
        EE_REFLECT_TYPE( VirtualParameterToolsNode );

    public:

        static VirtualParameterToolsNode* Create( FlowGraph* pRootGraph, GraphValueType type, String const& name, String const& category );

    public:

        VirtualParameterToolsNode();
        VirtualParameterToolsNode( String const& name, String const& groupName = String() );

        virtual char const* GetTypeName() const override { return "Parameter"; }
        virtual char const* GetCategory() const override { return "Virtual Parameters"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
    };

    //-------------------------------------------------------------------------

    class BoolVirtualParameterToolsNode final : public VirtualParameterToolsNode
    {
        EE_REFLECT_TYPE( BoolVirtualParameterToolsNode );

    public:

        BoolVirtualParameterToolsNode();
        BoolVirtualParameterToolsNode( String const& name, String const& groupName = String() );
    };

    //-------------------------------------------------------------------------

    class IDVirtualParameterToolsNode final : public VirtualParameterToolsNode
    {
        EE_REFLECT_TYPE( IDVirtualParameterToolsNode );

    public:

        IDVirtualParameterToolsNode();
        IDVirtualParameterToolsNode( String const& name, String const& groupName = String() );
    };

    //-------------------------------------------------------------------------

    class FloatVirtualParameterToolsNode final : public VirtualParameterToolsNode
    {
        EE_REFLECT_TYPE( FloatVirtualParameterToolsNode );

    public:

        FloatVirtualParameterToolsNode();
        FloatVirtualParameterToolsNode( String const& name, String const& groupName = String() );

    };

    //-------------------------------------------------------------------------

    class VectorVirtualParameterToolsNode final : public VirtualParameterToolsNode
    {
        EE_REFLECT_TYPE( VectorVirtualParameterToolsNode );

    public:

        VectorVirtualParameterToolsNode();
        VectorVirtualParameterToolsNode( String const& name, String const& groupName = String() );
    };

    //-------------------------------------------------------------------------

    class TargetVirtualParameterToolsNode final : public VirtualParameterToolsNode
    {
        EE_REFLECT_TYPE( TargetVirtualParameterToolsNode );

    public:

        TargetVirtualParameterToolsNode();
        TargetVirtualParameterToolsNode( String const& name, String const& groupName = String() );
    };

    //-------------------------------------------------------------------------

    class BoneMaskVirtualParameterToolsNode final : public VirtualParameterToolsNode
    {
        EE_REFLECT_TYPE( BoneMaskVirtualParameterToolsNode );

    public:

        BoneMaskVirtualParameterToolsNode();
        BoneMaskVirtualParameterToolsNode( String const& name, String const& groupName = String() );
    };

    //-------------------------------------------------------------------------
    // Parameter Reference
    //-------------------------------------------------------------------------

    class ParameterReferenceToolsNode : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ParameterReferenceToolsNode );

        friend ToolsGraphDefinition;

    public:

        static ParameterReferenceToolsNode* Create( FlowGraph* pGraph, ParameterBaseToolsNode* pParameter );

    public:

        ParameterReferenceToolsNode() = default;
        ParameterReferenceToolsNode( ParameterBaseToolsNode* pParameter );

        virtual char const* GetName() const override { return ( m_pParameter != nullptr ) ? m_pParameter->GetName() : FlowToolsNode::GetName(); }

        inline ParameterBaseToolsNode const* GetReferencedParameter() const { return m_pParameter; }
        inline UUID const& GetReferencedParameterID() const { return ( m_pParameter != nullptr ) ? m_pParameter->GetID() : m_parameterUUID; }
        inline char const* GetReferencedParameterName() const { return ( m_pParameter != nullptr ) ? m_pParameter->GetName() : m_parameterName.c_str(); }
        inline GraphValueType GetReferencedParameterValueType() const { return ( m_pParameter != nullptr ) ? m_pParameter->GetOutputValueType() : m_parameterValueType; }
        String const& GetReferencedParameterGroup() const { return ( m_pParameter != nullptr ) ? m_pParameter->GetParameterGroup() : m_parameterGroup; }

        inline bool IsReferencingControlParameter() const { return IsOfType<ControlParameterToolsNode>( m_pParameter ); }
        inline ControlParameterToolsNode const* GetReferencedControlParameter() const { return TryCast<ControlParameterToolsNode>( m_pParameter ); }
        inline ControlParameterToolsNode* GetReferencedControlParameter() { return TryCast<ControlParameterToolsNode>( m_pParameter ); }
        
        inline bool IsReferencingVirtualParameter() const { return IsOfType<VirtualParameterToolsNode>( m_pParameter ); }
        inline VirtualParameterToolsNode const* GetReferencedVirtualParameter() const { return TryCast<VirtualParameterToolsNode>( m_pParameter ); }
        inline VirtualParameterToolsNode* GetReferencedVirtualParameter() { return TryCast<VirtualParameterToolsNode>( m_pParameter ); }

        virtual char const* GetTypeName() const override { return "Parameter"; }
        virtual char const* GetCategory() const override { return "Parameter"; }
        virtual bool IsUserCreatable() const override { return true; }
        virtual bool IsDestroyable() const override { return true; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionConduit ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        void UpdateCachedParameterData();

        virtual void DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;
        virtual NodeGraph::BaseGraph* GetNavigationTarget() override;
        virtual void PreCopy() override { UpdateCachedParameterData(); }

        FlowToolsNode* GetDisplayValueNode();

    private:

        ParameterBaseToolsNode*                 m_pParameter = nullptr;

        // These values are cached and serialize to help with restoring references after serialization and for copy/pasting

        EE_REFLECT( ReadOnly );
        UUID                                    m_parameterUUID;

        EE_REFLECT( ReadOnly );
        GraphValueType                          m_parameterValueType;

        EE_REFLECT( ReadOnly );
        String                                  m_parameterName;

        EE_REFLECT( ReadOnly );
        String                                  m_parameterGroup;
    };

    //-------------------------------------------------------------------------

    class BoolParameterReferenceToolsNode final : public ParameterReferenceToolsNode
    {
        EE_REFLECT_TYPE( BoolParameterReferenceToolsNode );

    public:

        BoolParameterReferenceToolsNode() : ParameterReferenceToolsNode() { CreateOutputPin( "Value", GraphValueType::Bool, true ); }
        BoolParameterReferenceToolsNode( ParameterBaseToolsNode* pParameter ) : ParameterReferenceToolsNode( pParameter ) { CreateOutputPin( "Value", GraphValueType::Bool, true ); }
    };

    //-------------------------------------------------------------------------

    class IDParameterReferenceToolsNode final : public ParameterReferenceToolsNode
    {
        EE_REFLECT_TYPE( IDParameterReferenceToolsNode );

    public:

        IDParameterReferenceToolsNode() : ParameterReferenceToolsNode() { CreateOutputPin( "Value", GraphValueType::ID, true ); }
        IDParameterReferenceToolsNode( ParameterBaseToolsNode* pParameter ) : ParameterReferenceToolsNode( pParameter ) { CreateOutputPin( "Value", GraphValueType::ID, true ); }
    };

    //-------------------------------------------------------------------------

    class FloatParameterReferenceToolsNode final : public ParameterReferenceToolsNode
    {
        EE_REFLECT_TYPE( FloatParameterReferenceToolsNode );

    public:

        FloatParameterReferenceToolsNode() : ParameterReferenceToolsNode() { CreateOutputPin( "Value", GraphValueType::Float, true ); }
        FloatParameterReferenceToolsNode( ParameterBaseToolsNode* pParameter ) : ParameterReferenceToolsNode( pParameter ) { CreateOutputPin( "Value", GraphValueType::Float, true ); }
    };

    //-------------------------------------------------------------------------

    class VectorParameterReferenceToolsNode final : public ParameterReferenceToolsNode
    {
        EE_REFLECT_TYPE( VectorParameterReferenceToolsNode );

    public:

        VectorParameterReferenceToolsNode() : ParameterReferenceToolsNode() { CreateOutputPin( "Value", GraphValueType::Vector, true ); }
        VectorParameterReferenceToolsNode( ParameterBaseToolsNode* pParameter ) : ParameterReferenceToolsNode( pParameter ) { CreateOutputPin( "Value", GraphValueType::Vector, true ); }
    };

    //-------------------------------------------------------------------------

    class TargetParameterReferenceToolsNode final : public ParameterReferenceToolsNode
    {
        EE_REFLECT_TYPE( TargetParameterReferenceToolsNode );

    public:

        TargetParameterReferenceToolsNode() : ParameterReferenceToolsNode() { CreateOutputPin( "Value", GraphValueType::Target, true ); }
        TargetParameterReferenceToolsNode( ParameterBaseToolsNode* pParameter ) : ParameterReferenceToolsNode( pParameter ) { CreateOutputPin( "Value", GraphValueType::Target, true ); }
    };

    //-------------------------------------------------------------------------

    class BoneMaskParameterReferenceToolsNode final : public ParameterReferenceToolsNode
    {
        EE_REFLECT_TYPE( BoneMaskParameterReferenceToolsNode );

    public:

        BoneMaskParameterReferenceToolsNode() : ParameterReferenceToolsNode() { CreateOutputPin( "Value", GraphValueType::BoneMask, true ); }
        BoneMaskParameterReferenceToolsNode( ParameterBaseToolsNode* pParameter ) : ParameterReferenceToolsNode( pParameter ) { CreateOutputPin( "Value", GraphValueType::BoneMask, true ); }
    };
}