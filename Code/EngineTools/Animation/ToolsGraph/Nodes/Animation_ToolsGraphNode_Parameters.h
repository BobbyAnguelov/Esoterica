#pragma once
#include "Animation_ToolsGraphNode.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class ControlParameterToolsNode : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ControlParameterToolsNode );

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

    private:

        EE_REFLECT( "IsToolsReadOnly" : true ) String                     m_name;
        EE_REFLECT( "IsToolsReadOnly" : true ) String                     m_parameterCategory;
    };

    //-------------------------------------------------------------------------

    class VirtualParameterToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( VirtualParameterToolsNode );

    public:

        VirtualParameterToolsNode() = default;
        VirtualParameterToolsNode( GraphValueType type, String const& name, String const& categoryName = String() );

        inline String const& GetParameterName() const { return m_name; }
        void Rename( String const& name, String const& category );

        inline StringID GetParameterID() const { return StringID( m_name ); }
        inline String const& GetParameterCategory() const { return m_parameterCategory; }

        virtual GraphValueType GetValueType() const override { return m_type; }
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
        EE_REFLECT( "IsToolsReadOnly" : true ) GraphValueType             m_type = GraphValueType::Float;
    };

    //-------------------------------------------------------------------------

    class ParameterReferenceToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( ParameterReferenceToolsNode );

    public:

        ParameterReferenceToolsNode() = default;
        ParameterReferenceToolsNode( ControlParameterToolsNode* pParameter );
        ParameterReferenceToolsNode( VirtualParameterToolsNode* pParameter );

        virtual void Initialize( VisualGraph::BaseGraph* pParentGraph ) override;

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
        virtual GraphValueType GetValueType() const override { return GetReferencedParameterValueType(); }
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

    private:

        FlowToolsNode*                         m_pParameter = nullptr;

        // These values are cached and serialize to help with restoring references after serialization and for copy/pasting

        EE_REFLECT( "IsToolsReadOnly" : true ) UUID                       m_parameterUUID;
        EE_REFLECT( "IsToolsReadOnly" : true ) GraphValueType             m_parameterValueType;
        EE_REFLECT( "IsToolsReadOnly" : true ) String                     m_parameterName;
        EE_REFLECT( "IsToolsReadOnly" : true ) String                     m_parameterCategory;
    };

    //-------------------------------------------------------------------------

    class BoolControlParameterToolsNode : public ControlParameterToolsNode
    {
        EE_REFLECT_TYPE( BoolControlParameterToolsNode );

    public:

        using ControlParameterToolsNode::ControlParameterToolsNode;

        virtual void Initialize( VisualGraph::BaseGraph* pParentGraph ) override;

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

        using ControlParameterToolsNode::ControlParameterToolsNode;

        virtual void Initialize( VisualGraph::BaseGraph* pParentGraph ) override;

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

    class IntControlParameterToolsNode : public ControlParameterToolsNode
    {
        EE_REFLECT_TYPE( IntControlParameterToolsNode );

    public:

        using ControlParameterToolsNode::ControlParameterToolsNode;

        virtual void Initialize( VisualGraph::BaseGraph* pParentGraph ) override;

        inline int32_t GetPreviewStartValue() const { return m_previewStartValue; }

        virtual GraphValueType GetValueType() const override { return GraphValueType::Int; }

    private:

        EE_REFLECT() int32_t m_previewStartValue = 0;
    };

    //-------------------------------------------------------------------------

    class IDControlParameterToolsNode : public ControlParameterToolsNode
    {
        EE_REFLECT_TYPE( IDControlParameterToolsNode );

    public:

        using ControlParameterToolsNode::ControlParameterToolsNode;

        virtual void Initialize( VisualGraph::BaseGraph* pParentGraph ) override;

        inline StringID GetPreviewStartValue() const { return m_previewStartValue; }

        virtual GraphValueType GetValueType() const override { return GraphValueType::ID; }

    private:

        EE_REFLECT() StringID m_previewStartValue;
    };

    //-------------------------------------------------------------------------

    class VectorControlParameterToolsNode : public ControlParameterToolsNode
    {
        EE_REFLECT_TYPE( VectorControlParameterToolsNode );

    public:

        using ControlParameterToolsNode::ControlParameterToolsNode;

        virtual void Initialize( VisualGraph::BaseGraph* pParentGraph ) override;

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

        using ControlParameterToolsNode::ControlParameterToolsNode;

        virtual void Initialize( VisualGraph::BaseGraph* pParentGraph ) override;

        inline Target GetPreviewStartValue() const
        {
            Target target;

            if ( m_isSet )
            {
                if ( m_isBoneID )
                {
                    target = Target( m_previewStartBoneID );
                }
                else
                {
                    target = Target( m_previewStartTransform );
                }
            }

            return target;
        }

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
}