#pragma once
#include "Animation_ToolsGraphNode.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class FlowGraph;
}

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    //-------------------------------------------------------------------------
    // Control Parameter
    //-------------------------------------------------------------------------

    class ControlParameterToolsNode : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ControlParameterToolsNode );

    public:

        static ControlParameterToolsNode* Create( FlowGraph* pRootGraph, GraphValueType type, String const& name, String const& category );

    public:

        ControlParameterToolsNode() = default;
        ControlParameterToolsNode( String const& name, String const& categoryName = String() );

        inline String const& GetParameterName() const { return m_name; }
        void Rename( String const& name, String const& category );

        inline StringID GetParameterID() const { return StringID( m_name ); }
        inline String const& GetParameterCategory() const { return m_parameterCategory; }

        virtual bool IsVisible() const override { return false; }
        virtual char const* GetName() const override { return m_name.c_str(); }
        virtual char const* GetTypeName() const override { return "Parameter"; }
        virtual char const* GetCategory() const override { return "Control Parameters"; }
        virtual bool IsUserCreatable() const override { return false; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual bool IsPersistentNode() const override { return true; }

        virtual void ReflectPreviewValues( ControlParameterToolsNode const* pOtherParameterNode ) {}

    private:

        EE_REFLECT( "IsToolsReadOnly" : true ) String                     m_name;
        EE_REFLECT( "IsToolsReadOnly" : true ) String                     m_parameterCategory;
    };

    //-------------------------------------------------------------------------

    class BoolControlParameterToolsNode : public ControlParameterToolsNode
    {
        EE_REFLECT_TYPE( BoolControlParameterToolsNode );

    public:

        BoolControlParameterToolsNode();
        BoolControlParameterToolsNode( String const& name, String const& categoryName = String() );

        inline bool GetPreviewStartValue() const { return m_previewStartValue; }

        virtual GraphValueType GetValueType() const override { return GraphValueType::Bool; }

    private:

        EE_REFLECT() bool m_previewStartValue = false;
    };

    //-------------------------------------------------------------------------

    class FloatControlParameterToolsNode : public ControlParameterToolsNode
    {
        EE_REFLECT_TYPE( FloatControlParameterToolsNode );

    public:

        FloatControlParameterToolsNode();
        FloatControlParameterToolsNode( String const& name, String const& categoryName = String() );

        inline float GetPreviewStartValue() const { return m_previewStartValue; }
        inline float GetPreviewRangeMin() const { return m_previewMin; }
        inline float GetPreviewRangeMax() const { return m_previewMax; }

        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }

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
        IDControlParameterToolsNode( String const& name, String const& categoryName = String() );

        inline StringID GetPreviewStartValue() const { return m_previewStartValue; }

        virtual GraphValueType GetValueType() const override { return GraphValueType::ID; }

        virtual void GetLogicAndEventIDs( TVector<StringID>& outIDs ) const override;
        virtual void RenameLogicAndEventIDs( StringID oldID, StringID newID ) override;
        virtual void ReflectPreviewValues( ControlParameterToolsNode const* pOtherParameterNode ) override;

        // Get all the expected preview values for this parameter
        TVector<StringID> const& GetExpectedPreviewValues() const { return m_expectedValues; }

    private:

        EE_REFLECT( "CustomEditor" : "AnimGraph_ID" );
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
        VectorControlParameterToolsNode( String const& name, String const& categoryName = String() );

        inline Vector GetPreviewStartValue() const { return m_previewStartValue; }

        virtual GraphValueType GetValueType() const override { return GraphValueType::Vector; }

    private:

        EE_REFLECT() Vector m_previewStartValue = Vector::Zero;
    };

    //-------------------------------------------------------------------------

    class TargetControlParameterToolsNode : public ControlParameterToolsNode
    {
        EE_REFLECT_TYPE( TargetControlParameterToolsNode );

    public:

        TargetControlParameterToolsNode();
        TargetControlParameterToolsNode( String const& name, String const& categoryName = String() );

        Target GetPreviewStartValue() const;
        inline bool IsBoneTarget() const { return m_isBoneID; }
        inline Transform const& GetPreviewTargetTransform() const { return m_previewStartTransform; }
        void SetPreviewTargetTransform( Transform const& transform );

        virtual GraphValueType GetValueType() const override { return GraphValueType::Target; }

    private:

        EE_REFLECT() bool         m_isSet = false;
        EE_REFLECT() bool         m_isBoneID = false;
        EE_REFLECT() Transform    m_previewStartTransform = Transform::Identity;
        EE_REFLECT() StringID     m_previewStartBoneID;
    };

    //-------------------------------------------------------------------------
    // Virtual Parameter
    //-------------------------------------------------------------------------

    class VirtualParameterToolsNode : public FlowToolsNode
    {
        EE_REFLECT_TYPE( VirtualParameterToolsNode );

    public:

        static VirtualParameterToolsNode* Create( FlowGraph* pRootGraph, GraphValueType type, String const& name, String const& category );

    public:

        VirtualParameterToolsNode() = default;
        VirtualParameterToolsNode( String const& name, String const& categoryName = String() );

        inline String const& GetParameterName() const { return m_name; }
        void Rename( String const& name, String const& category );

        inline StringID GetParameterID() const { return StringID( m_name ); }
        inline String const& GetParameterCategory() const { return m_parameterCategory; }

        virtual void Initialize( VisualGraph::BaseGraph* pParentGraph ) override;
        virtual bool IsVisible() const override { return false; }
        virtual char const* GetName() const override { return m_name.c_str(); }
        virtual char const* GetTypeName() const override { return "Parameter"; }
        virtual char const* GetCategory() const override { return "Virtual Parameters"; }
        virtual bool IsUserCreatable() const override { return false; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_REFLECT( "IsToolsReadOnly" : true ) String                     m_name;
        EE_REFLECT( "IsToolsReadOnly" : true ) String                     m_parameterCategory;
    };

    //-------------------------------------------------------------------------

    class BoolVirtualParameterToolsNode final : public VirtualParameterToolsNode
    {
        EE_REFLECT_TYPE( BoolVirtualParameterToolsNode );

    public:

        BoolVirtualParameterToolsNode() : VirtualParameterToolsNode() { CreateOutputPin( "Value", GraphValueType::Bool, true ); }
        BoolVirtualParameterToolsNode( String const& name, String const& categoryName = String() ) : VirtualParameterToolsNode( name, categoryName ) { CreateOutputPin( "Value", GraphValueType::Bool, true ); }
        virtual GraphValueType GetValueType() const override { return GraphValueType::Bool; }
    };

    //-------------------------------------------------------------------------

    class IDVirtualParameterToolsNode final : public VirtualParameterToolsNode
    {
        EE_REFLECT_TYPE( IDVirtualParameterToolsNode );

    public:

        IDVirtualParameterToolsNode() : VirtualParameterToolsNode() { CreateOutputPin( "Value", GraphValueType::ID, true ); }
        IDVirtualParameterToolsNode( String const& name, String const& categoryName = String() ) : VirtualParameterToolsNode( name, categoryName ) { CreateOutputPin( "Value", GraphValueType::ID, true ); }
        virtual GraphValueType GetValueType() const override { return GraphValueType::ID; }
    };

    //-------------------------------------------------------------------------

    class FloatVirtualParameterToolsNode final : public VirtualParameterToolsNode
    {
        EE_REFLECT_TYPE( FloatVirtualParameterToolsNode );

    public:

        FloatVirtualParameterToolsNode() : VirtualParameterToolsNode() { CreateOutputPin( "Value", GraphValueType::Float, true ); }
        FloatVirtualParameterToolsNode( String const& name, String const& categoryName = String() ) : VirtualParameterToolsNode( name, categoryName ) { CreateOutputPin( "Value", GraphValueType::Float, true ); }
        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
    };

    //-------------------------------------------------------------------------

    class VectorVirtualParameterToolsNode final : public VirtualParameterToolsNode
    {
        EE_REFLECT_TYPE( VectorVirtualParameterToolsNode );

    public:

        VectorVirtualParameterToolsNode() : VirtualParameterToolsNode() { CreateOutputPin( "Value", GraphValueType::Vector, true ); }
        VectorVirtualParameterToolsNode( String const& name, String const& categoryName = String() ) : VirtualParameterToolsNode( name, categoryName ) { CreateOutputPin( "Value", GraphValueType::Vector, true ); }
        virtual GraphValueType GetValueType() const override { return GraphValueType::Vector; }
    };

    //-------------------------------------------------------------------------

    class TargetVirtualParameterToolsNode final : public VirtualParameterToolsNode
    {
        EE_REFLECT_TYPE( TargetVirtualParameterToolsNode );

    public:

        TargetVirtualParameterToolsNode() : VirtualParameterToolsNode() { CreateOutputPin( "Value", GraphValueType::Target, true ); }
        TargetVirtualParameterToolsNode( String const& name, String const& categoryName = String() ) : VirtualParameterToolsNode( name, categoryName ) { CreateOutputPin( "Value", GraphValueType::Target, true ); }
        virtual GraphValueType GetValueType() const override { return GraphValueType::Target; }
    };

    //-------------------------------------------------------------------------

    class BoneMaskVirtualParameterToolsNode final : public VirtualParameterToolsNode
    {
        EE_REFLECT_TYPE( BoneMaskVirtualParameterToolsNode );

    public:

        BoneMaskVirtualParameterToolsNode() : VirtualParameterToolsNode() { CreateOutputPin( "Value", GraphValueType::BoneMask, true ); }
        BoneMaskVirtualParameterToolsNode( String const& name, String const& categoryName = String() ) : VirtualParameterToolsNode( name, categoryName ) { CreateOutputPin( "Value", GraphValueType::BoneMask, true ); }
        virtual GraphValueType GetValueType() const override { return GraphValueType::BoneMask; }
    };

    //-------------------------------------------------------------------------
    // Parameter Reference
    //-------------------------------------------------------------------------

    class ParameterReferenceToolsNode : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ParameterReferenceToolsNode );

    public:

        ParameterReferenceToolsNode() = default;
        ParameterReferenceToolsNode( ControlParameterToolsNode* pParameter );
        ParameterReferenceToolsNode( VirtualParameterToolsNode* pParameter );

        void SetReferencedParameter( FlowToolsNode* pParameter );
        inline FlowToolsNode const* GetReferencedParameter() const { return m_pParameter; }
        inline UUID const& GetReferencedParameterID() const { return ( m_pParameter != nullptr ) ? m_pParameter->GetID() : m_parameterUUID; }
        inline GraphValueType GetReferencedParameterValueType() const { return ( m_pParameter != nullptr ) ? m_pParameter->GetValueType() : m_parameterValueType; }
        inline char const* GetReferencedParameterName() const { return ( m_pParameter != nullptr ) ? m_pParameter->GetName() : m_parameterName.c_str(); }
        String const& GetReferencedParameterCategory() const;

        inline bool IsReferencingControlParameter() const { return IsOfType<ControlParameterToolsNode>( m_pParameter ); }
        inline ControlParameterToolsNode const* GetReferencedControlParameter() const { return TryCast<ControlParameterToolsNode>( m_pParameter ); }
        inline ControlParameterToolsNode* GetReferencedControlParameter() { return TryCast<ControlParameterToolsNode>( m_pParameter ); }
        
        inline bool IsReferencingVirtualParameter() const { return IsOfType<VirtualParameterToolsNode>( m_pParameter ); }
        inline VirtualParameterToolsNode const* GetReferencedVirtualParameter() const { return TryCast<VirtualParameterToolsNode>( m_pParameter ); }
        inline VirtualParameterToolsNode* GetReferencedVirtualParameter() { return TryCast<VirtualParameterToolsNode>( m_pParameter ); }

        virtual char const* GetName() const override { return m_pParameter->GetName(); }
        virtual char const* GetTypeName() const override { return "Parameter"; }
        virtual char const* GetCategory() const override { return "Parameter"; }
        virtual bool IsUserCreatable() const override { return true; }
        virtual bool IsDestroyable() const override { return true; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        void UpdateCachedParameterData();

        virtual void DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext ) override;
        virtual void OnDoubleClick( VisualGraph::UserContext* pUserContext ) override;
        virtual void PrepareForCopy() override { UpdateCachedParameterData(); }

        FlowToolsNode* GetDisplayValueNode();

    private:

        FlowToolsNode*                                                    m_pParameter = nullptr;

        // These values are cached and serialize to help with restoring references after serialization and for copy/pasting

        EE_REFLECT( "IsToolsReadOnly" : true ) UUID                       m_parameterUUID;
        EE_REFLECT( "IsToolsReadOnly" : true ) GraphValueType             m_parameterValueType;
        EE_REFLECT( "IsToolsReadOnly" : true ) String                     m_parameterName;
        EE_REFLECT( "IsToolsReadOnly" : true ) String                     m_parameterCategory;
    };

    //-------------------------------------------------------------------------

    class BoolParameterReferenceToolsNode final : public ParameterReferenceToolsNode
    {
        EE_REFLECT_TYPE( BoolParameterReferenceToolsNode );

    public:

        BoolParameterReferenceToolsNode() : ParameterReferenceToolsNode() { CreateOutputPin( "Value", GraphValueType::Bool, true ); }
        BoolParameterReferenceToolsNode( ControlParameterToolsNode* pParameter ) : ParameterReferenceToolsNode( pParameter ) { CreateOutputPin( "Value", GraphValueType::Bool, true ); }
        BoolParameterReferenceToolsNode( VirtualParameterToolsNode* pParameter ) : ParameterReferenceToolsNode( pParameter ) { CreateOutputPin( "Value", GraphValueType::Bool, true ); }
        virtual GraphValueType GetValueType() const override { return GraphValueType::Bool; }
    };

    //-------------------------------------------------------------------------

    class IDParameterReferenceToolsNode final : public ParameterReferenceToolsNode
    {
        EE_REFLECT_TYPE( IDParameterReferenceToolsNode );

    public:

        IDParameterReferenceToolsNode() : ParameterReferenceToolsNode() { CreateOutputPin( "Value", GraphValueType::ID, true ); }
        IDParameterReferenceToolsNode( ControlParameterToolsNode* pParameter ) : ParameterReferenceToolsNode( pParameter ) { CreateOutputPin( "Value", GraphValueType::ID, true ); }
        IDParameterReferenceToolsNode( VirtualParameterToolsNode* pParameter ) : ParameterReferenceToolsNode( pParameter ) { CreateOutputPin( "Value", GraphValueType::ID, true ); }
        virtual GraphValueType GetValueType() const override { return GraphValueType::ID; }
    };

    //-------------------------------------------------------------------------

    class FloatParameterReferenceToolsNode final : public ParameterReferenceToolsNode
    {
        EE_REFLECT_TYPE( FloatParameterReferenceToolsNode );

    public:

        FloatParameterReferenceToolsNode() : ParameterReferenceToolsNode() { CreateOutputPin( "Value", GraphValueType::Float, true ); }
        FloatParameterReferenceToolsNode( ControlParameterToolsNode* pParameter ) : ParameterReferenceToolsNode( pParameter ) { CreateOutputPin( "Value", GraphValueType::Float, true ); }
        FloatParameterReferenceToolsNode( VirtualParameterToolsNode* pParameter ) : ParameterReferenceToolsNode( pParameter ) { CreateOutputPin( "Value", GraphValueType::Float, true ); }
        virtual GraphValueType GetValueType() const override { return GraphValueType::Float; }
    };

    //-------------------------------------------------------------------------

    class VectorParameterReferenceToolsNode final : public ParameterReferenceToolsNode
    {
        EE_REFLECT_TYPE( VectorParameterReferenceToolsNode );

    public:

        VectorParameterReferenceToolsNode() : ParameterReferenceToolsNode() { CreateOutputPin( "Value", GraphValueType::Vector, true ); }
        VectorParameterReferenceToolsNode( ControlParameterToolsNode* pParameter ) : ParameterReferenceToolsNode( pParameter ) { CreateOutputPin( "Value", GraphValueType::Vector, true ); }
        VectorParameterReferenceToolsNode( VirtualParameterToolsNode* pParameter ) : ParameterReferenceToolsNode( pParameter ) { CreateOutputPin( "Value", GraphValueType::Vector, true ); }
        virtual GraphValueType GetValueType() const override { return GraphValueType::Vector; }
    };

    //-------------------------------------------------------------------------

    class TargetParameterReferenceToolsNode final : public ParameterReferenceToolsNode
    {
        EE_REFLECT_TYPE( TargetParameterReferenceToolsNode );

    public:

        TargetParameterReferenceToolsNode() : ParameterReferenceToolsNode() { CreateOutputPin( "Value", GraphValueType::Target, true ); }
        TargetParameterReferenceToolsNode( ControlParameterToolsNode* pParameter ) : ParameterReferenceToolsNode( pParameter ) { CreateOutputPin( "Value", GraphValueType::Target, true ); }
        TargetParameterReferenceToolsNode( VirtualParameterToolsNode* pParameter ) : ParameterReferenceToolsNode( pParameter ) { CreateOutputPin( "Value", GraphValueType::Target, true ); }
        virtual GraphValueType GetValueType() const override { return GraphValueType::Target; }
    };

    //-------------------------------------------------------------------------

    class BoneMaskParameterReferenceToolsNode final : public ParameterReferenceToolsNode
    {
        EE_REFLECT_TYPE( BoneMaskParameterReferenceToolsNode );

    public:

        BoneMaskParameterReferenceToolsNode() : ParameterReferenceToolsNode() { CreateOutputPin( "Value", GraphValueType::BoneMask, true ); }
        BoneMaskParameterReferenceToolsNode( ControlParameterToolsNode* pParameter ) : ParameterReferenceToolsNode( pParameter ) { CreateOutputPin( "Value", GraphValueType::BoneMask, true ); }
        BoneMaskParameterReferenceToolsNode( VirtualParameterToolsNode* pParameter ) : ParameterReferenceToolsNode( pParameter ) { CreateOutputPin( "Value", GraphValueType::BoneMask, true ); }
        virtual GraphValueType GetValueType() const override { return GraphValueType::BoneMask; }
    };
}