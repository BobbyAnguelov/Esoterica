#pragma once
#include "Animation_ToolsGraphNode.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class ControlParameterToolsNode : public FlowToolsNode
    {
        EE_REGISTER_TYPE( ControlParameterToolsNode );

    public:

        ControlParameterToolsNode() = default;
        ControlParameterToolsNode( String const& name );

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

        EE_REGISTER String                     m_name;
        EE_REGISTER String                     m_parameterCategory;
    };

    //-------------------------------------------------------------------------

    class VirtualParameterToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( VirtualParameterToolsNode );

    public:

        VirtualParameterToolsNode() = default;
        VirtualParameterToolsNode( String const& name, GraphValueType type );

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

        EE_REGISTER String                     m_name;
        EE_REGISTER String                     m_parameterCategory;
        EE_REGISTER GraphValueType             m_type = GraphValueType::Float;
    };

    //-------------------------------------------------------------------------

    class ParameterReferenceToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( ParameterReferenceToolsNode );

    public:

        static void GloballyRefreshParameterReferences( VisualGraph::BaseGraph* pRootGraph );

    public:

        ParameterReferenceToolsNode() = default;
        ParameterReferenceToolsNode( ControlParameterToolsNode* pParameter );
        ParameterReferenceToolsNode( VirtualParameterToolsNode* pParameter );

        virtual void Initialize( VisualGraph::BaseGraph* pParentGraph ) override;

        inline FlowToolsNode const* GetReferencedParameter() const { return m_pParameter; }
        inline ControlParameterToolsNode const* GetReferencedControlParameter() const { return TryCast<ControlParameterToolsNode>( m_pParameter ); }
        inline ControlParameterToolsNode* GetReferencedControlParameter() { return TryCast<ControlParameterToolsNode>( m_pParameter ); }
        inline bool IsReferencingControlParameter() const { return IsOfType<ControlParameterToolsNode>( m_pParameter ); }
        inline VirtualParameterToolsNode const* GetReferencedVirtualParameter() const { return TryCast<VirtualParameterToolsNode>( m_pParameter ); }
        inline VirtualParameterToolsNode* GetReferencedVirtualParameter() { return TryCast<VirtualParameterToolsNode>( m_pParameter ); }
        inline bool IsReferencingVirtualParameter() const { return IsOfType<VirtualParameterToolsNode>( m_pParameter ); }
        inline UUID const& GetReferencedParameterID() const { return m_parameterUUID; }
        inline GraphValueType GetParameterValueType() const { return m_parameterValueType; }

        virtual char const* GetName() const override { return m_pParameter->GetName(); }
        virtual char const* GetTypeName() const override { return "Parameter"; }
        virtual char const* GetCategory() const override { return "Parameter"; }
        virtual bool IsUserCreatable() const override { return true; }
        virtual bool IsDestroyable() const override { return true; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree, GraphType::TransitionTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        FlowToolsNode*                         m_pParameter = nullptr;
        EE_REGISTER UUID                       m_parameterUUID;
        EE_REGISTER GraphValueType             m_parameterValueType;
    };

    //-------------------------------------------------------------------------

    class BoolControlParameterToolsNode : public ControlParameterToolsNode
    {
        EE_REGISTER_TYPE( BoolControlParameterToolsNode );

    public:

        using ControlParameterToolsNode::ControlParameterToolsNode;

        virtual void Initialize( VisualGraph::BaseGraph* pParentGraph ) override;

        inline bool GetPreviewStartValue() const { return m_previewStartValue; }

    private:

        EE_EXPOSE bool m_previewStartValue = false;
    };

    //-------------------------------------------------------------------------

    class FloatControlParameterToolsNode : public ControlParameterToolsNode
    {
        EE_REGISTER_TYPE( FloatControlParameterToolsNode );

    public:

        using ControlParameterToolsNode::ControlParameterToolsNode;

        virtual void Initialize( VisualGraph::BaseGraph* pParentGraph ) override;

        inline float GetPreviewStartValue() const { return m_previewStartValue; }
        inline float GetPreviewRangeMin() const { return m_previewMin; }
        inline float GetPreviewRangeMax() const { return m_previewMax; }

    private:

        EE_EXPOSE float m_previewStartValue = 0;
        EE_EXPOSE float m_previewMin = 0;
        EE_EXPOSE float m_previewMax = 1;
    };

    //-------------------------------------------------------------------------

    class IntControlParameterToolsNode : public ControlParameterToolsNode
    {
        EE_REGISTER_TYPE( IntControlParameterToolsNode );

    public:

        using ControlParameterToolsNode::ControlParameterToolsNode;

        virtual void Initialize( VisualGraph::BaseGraph* pParentGraph ) override;

        inline int32_t GetPreviewStartValue() const { return m_previewStartValue; }

    private:

        EE_EXPOSE int32_t m_previewStartValue = 0;
    };

    //-------------------------------------------------------------------------

    class IDControlParameterToolsNode : public ControlParameterToolsNode
    {
        EE_REGISTER_TYPE( IDControlParameterToolsNode );

    public:

        using ControlParameterToolsNode::ControlParameterToolsNode;

        virtual void Initialize( VisualGraph::BaseGraph* pParentGraph ) override;

        inline StringID GetPreviewStartValue() const { return m_previewStartValue; }

    private:

        EE_EXPOSE StringID m_previewStartValue;
    };

    //-------------------------------------------------------------------------

    class VectorControlParameterToolsNode : public ControlParameterToolsNode
    {
        EE_REGISTER_TYPE( VectorControlParameterToolsNode );

    public:

        using ControlParameterToolsNode::ControlParameterToolsNode;

        virtual void Initialize( VisualGraph::BaseGraph* pParentGraph ) override;

        inline Vector GetPreviewStartValue() const { return m_previewStartValue; }

    private:

        EE_EXPOSE Vector m_previewStartValue = Vector::Zero;
    };

    //-------------------------------------------------------------------------

    class TargetControlParameterToolsNode : public ControlParameterToolsNode
    {
        EE_REGISTER_TYPE( TargetControlParameterToolsNode );

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

    private:

        EE_EXPOSE bool         m_isSet = false;
        EE_EXPOSE bool         m_isBoneID = false;
        EE_EXPOSE Transform    m_previewStartTransform = Transform::Identity;
        EE_EXPOSE StringID     m_previewStartBoneID;
    };
}