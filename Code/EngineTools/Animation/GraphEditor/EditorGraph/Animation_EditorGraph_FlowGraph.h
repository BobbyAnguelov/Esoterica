#pragma once
#include "Animation_EditorGraph_Common.h"
#include "EngineTools/Core/VisualGraph/VisualGraph_FlowGraph.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphCompilationContext;
    class EditorGraphDefinition;
    class VariationHierarchy;

    //-------------------------------------------------------------------------
    // Base Animation Flow Graph Node
    //-------------------------------------------------------------------------

    namespace GraphNodes
    {
        class EditorGraphNode : public VisualGraph::Flow::Node
        {
            EE_REGISTER_TYPE( EditorGraphNode );

        public:

            using VisualGraph::Flow::Node::Node;

            // Get the type of node this is, this is either the output type for the nodes with output or the first input for nodes with no outputs
            EE_FORCE_INLINE GraphValueType GetValueType() const
            {
                if ( GetNumOutputPins() > 0 )
                {
                    return GraphValueType( GetOutputPin( 0 )->m_type );
                }
                else if ( GetNumInputPins() > 0 )
                {
                    return GraphValueType( GetInputPin( 0 )->m_type );
                }
                else
                {
                    return GraphValueType::Unknown;
                }
            }

            virtual ImColor GetTitleBarColor() const override;
            virtual ImColor GetPinColor( VisualGraph::Flow::Pin const& pin ) const override;

            // Get the types of graphs that this node is allowed to be placed in
            virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const = 0;

            // Is this node a persistent node i.e. is it always initialized 
            virtual bool IsPersistentNode() const { return false; }

            // Compile this node into its runtime representation. Returns the node index of the compiled node.
            virtual int16_t Compile( GraphCompilationContext& context ) const { return int16_t(); }

        protected:

            EE_FORCE_INLINE void CreateInputPin( char const* pPinName, GraphValueType pinType ) { VisualGraph::Flow::Node::CreateInputPin( pPinName, (uint32_t) pinType ); }
            EE_FORCE_INLINE void CreateOutputPin( char const* pPinName, GraphValueType pinType, bool allowMultipleOutputConnections = false ) { VisualGraph::Flow::Node::CreateOutputPin( pPinName, (uint32_t) pinType, allowMultipleOutputConnections ); }

            virtual bool IsActive( VisualGraph::DrawContext const& ctx ) const override;

            virtual void DrawExtraControls( VisualGraph::DrawContext const& ctx ) override;

            // Draw any extra information about the node
            virtual void DrawInfoText( VisualGraph::DrawContext const& ctx ) {}
        };

        //-------------------------------------------------------------------------
        // Data Slot Nodes
        //-------------------------------------------------------------------------

        class DataSlotEditorNode : public EditorGraphNode
        {
            EE_REGISTER_TYPE( DataSlotEditorNode );

        public:

            struct OverrideValue : public IRegisteredType
            {
                EE_REGISTER_TYPE( OverrideValue );

                EE_REGISTER StringID               m_variationID;
                EE_REGISTER ResourceID             m_resourceID;
            };

        public:

            virtual char const* GetName() const override { return m_name.c_str(); }
            virtual bool IsRenamable() const override { return true; }
            virtual void SetName( String const& newName ) override;

            virtual void DrawExtraControls( VisualGraph::DrawContext const& ctx ) override;
            virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;
            virtual void PostPaste() override;

            #if EE_DEVELOPMENT_TOOLS
            virtual void PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited ) override;
            #endif

            // Slot
            //-------------------------------------------------------------------------

            virtual char const* const GetDefaultSlotName() const { return "Slot"; }

            bool AreSlotValuesValid() const;

            virtual ResourceTypeID GetSlotResourceTypeID() const { return ResourceTypeID(); }

            // This will return the final resolved resource value for this slot
            ResourceID GetResourceID( VariationHierarchy const& variationHierarchy, StringID variationID ) const;

            // This sets the resource for the default variation
            void SetDefaultResourceID( ResourceID const& defaultResourceID )
            {
                EE_ASSERT( defaultResourceID.GetResourceTypeID() == GetSlotResourceTypeID() );
                m_defaultResourceID = defaultResourceID;
            }

            // Variation override management
            //-------------------------------------------------------------------------

            bool HasOverrideForVariation( StringID variationID ) const { return GetOverrideValueForVariation( variationID ) != nullptr; }
            ResourceID* GetOverrideValueForVariation( StringID variationID );
            ResourceID const* GetOverrideValueForVariation( StringID variationID ) const { return const_cast<DataSlotEditorNode*>( this )->GetOverrideValueForVariation( variationID ); }

            void CreateOverride( StringID variationID );
            void RenameOverride( StringID oldVariationID, StringID newVariationID );
            void RemoveOverride( StringID variationID );
            void SetOverrideValueForVariation( StringID variationID, ResourceID const& resourceID );

        private:

            String GetUniqueSlotName( String const& desiredName );

        protected:

            EE_EXPOSE String                       m_name;
            EE_REGISTER ResourceID                 m_defaultResourceID;
            EE_REGISTER TVector<OverrideValue>     m_overrides;
        };

        //-------------------------------------------------------------------------
        // Result Node
        //-------------------------------------------------------------------------

        class ResultEditorNode final : public EditorGraphNode
        {
            EE_REGISTER_TYPE( ResultEditorNode );

        public:

            ResultEditorNode() = default;
            ResultEditorNode( GraphValueType valueType );

            virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

            virtual char const* GetTypeName() const override { return "Result"; }
            virtual char const* GetCategory() const override { return "Results"; }
            virtual bool IsUserCreatable() const override { return false; }
            virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree, GraphType::ValueTree ); }
            virtual int16_t Compile( GraphCompilationContext& context ) const override;

        private:

            EE_REGISTER GraphValueType m_valueType = GraphValueType::Pose;
        };

        //-------------------------------------------------------------------------
        // Parameters
        //-------------------------------------------------------------------------

        class ControlParameterEditorNode : public EditorGraphNode
        {
            EE_REGISTER_TYPE( ControlParameterEditorNode );
            friend EditorGraphDefinition;

        public:

            ControlParameterEditorNode() = default;
            ControlParameterEditorNode( String const& name );

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

        class VirtualParameterEditorNode final : public EditorGraphNode
        {
            EE_REGISTER_TYPE( VirtualParameterEditorNode );
            friend EditorGraphDefinition;

        public:

            VirtualParameterEditorNode() = default;
            VirtualParameterEditorNode( String const& name, GraphValueType type );

            inline String const& GetParameterName() const { return m_name; }
            void Rename( String const& name, String const& category );

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

        class ParameterReferenceEditorNode final : public EditorGraphNode
        {
            EE_REGISTER_TYPE( ParameterReferenceEditorNode );
            friend EditorGraphDefinition;

        public:

            static void RefreshParameterReferences( VisualGraph::BaseGraph* pRootGraph );

        public:

            ParameterReferenceEditorNode() = default;
            ParameterReferenceEditorNode( ControlParameterEditorNode* pParameter );
            ParameterReferenceEditorNode( VirtualParameterEditorNode* pParameter );

            virtual void Initialize( VisualGraph::BaseGraph* pParentGraph ) override;

            inline EditorGraphNode const* GetReferencedParameter() const { return m_pParameter; }
            inline ControlParameterEditorNode const* GetReferencedControlParameter() const { return TryCast<ControlParameterEditorNode>( m_pParameter ); }
            inline ControlParameterEditorNode* GetReferencedControlParameter() { return TryCast<ControlParameterEditorNode>( m_pParameter ); }
            inline bool IsReferencingControlParameter() const { return IsOfType<ControlParameterEditorNode>( m_pParameter ); }
            inline VirtualParameterEditorNode const* GetReferencedVirtualParameter() const { return TryCast<VirtualParameterEditorNode>( m_pParameter ); }
            inline VirtualParameterEditorNode* GetReferencedVirtualParameter() { return TryCast<VirtualParameterEditorNode>( m_pParameter ); }
            inline bool IsReferencingVirtualParameter() const { return IsOfType<VirtualParameterEditorNode>( m_pParameter ); }
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

            EditorGraphNode*                        m_pParameter = nullptr;
            EE_REGISTER UUID                       m_parameterUUID;
            EE_REGISTER GraphValueType             m_parameterValueType;
        };
    }

    //-------------------------------------------------------------------------
    // Base Animation Flow Graph
    //-------------------------------------------------------------------------

    class FlowGraph : public VisualGraph::FlowGraph
    {
        friend class EditorGraphDefinition;
        EE_REGISTER_TYPE( FlowGraph );

    public:

        FlowGraph( GraphType type = GraphType::BlendTree );

        inline GraphType GetType() const { return m_type; }

        // Node factory methods
        //-------------------------------------------------------------------------

        template<typename T, typename ... ConstructorParams>
        T* CreateNode( ConstructorParams&&... params )
        {
            VisualGraph::ScopedGraphModification sgm( this );

            static_assert( std::is_base_of<GraphNodes::EditorGraphNode, T>::value );
            auto pNode = EE::New<T>( std::forward<ConstructorParams>( params )... );
            EE_ASSERT( pNode->GetAllowedParentGraphTypes().IsFlagSet( m_type ) );
            pNode->Initialize( this );
            AddNode( pNode );
            return pNode;
        }

        GraphNodes::EditorGraphNode* CreateNode( TypeSystem::TypeInfo const* pTypeInfo )
        {
            VisualGraph::ScopedGraphModification sgm( this );

            EE_ASSERT( pTypeInfo->IsDerivedFrom( GraphNodes::EditorGraphNode::GetStaticTypeID() ) );
            auto pNode = Cast<GraphNodes::EditorGraphNode>( pTypeInfo->CreateType() );
            EE_ASSERT( pNode->GetAllowedParentGraphTypes().IsFlagSet( m_type ) );
            pNode->Initialize( this );
            AddNode( pNode );
            return pNode;
        }

    private:

        virtual void PostPasteNodes( TInlineVector<VisualGraph::BaseNode*, 20> const& pastedNodes ) override;
        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& graphObjectValue ) override;
        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const override;

    private:

        EE_REGISTER GraphType       m_type;
    };
}