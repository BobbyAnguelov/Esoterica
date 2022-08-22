#pragma once
#include "EditorGraph/Animation_EditorGraph_Definition.h"
#include "EngineTools/Core/VisualGraph/VisualGraph_View.h"
#include "EngineTools/Core/Helpers/CategoryTree.h"

//-------------------------------------------------------------------------

namespace EE
{
    class ToolsContext;
}

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphEditorContext
    {
    public:

        GraphEditorContext( ToolsContext const& toolsContext );

        inline ToolsContext const* GetToolsContext() const { return &m_toolsContext; }

        // Load an existing graph
        bool LoadGraph( Serialization::JsonValue const& graphDescriptorObjectValue );

        // Saves this graph
        void SaveGraph( Serialization::JsonWriter& writer ) const;

        // Dirty State
        //-------------------------------------------------------------------------

        inline bool IsDirty() const { return m_editorGraph.IsDirty(); }
        inline void MarkDirty() { m_editorGraph.MarkDirty(); }
        inline void ClearDirty() { m_editorGraph.ClearDirty(); }

        void BeginGraphModification() { m_editorGraph.MarkDirty(); m_editorGraph.GetRootGraph()->BeginModification(); }
        void EndGraphModification() { m_editorGraph.GetRootGraph()->EndModification(); }

        // Graph Data
        //-------------------------------------------------------------------------

        EditorGraphDefinition const* GetGraphDefinition() const { return &m_editorGraph; }

        EditorGraphDefinition* GetGraphDefinition() { return &m_editorGraph; }

        inline FlowGraph* GetRootGraph() { return m_editorGraph.GetRootGraph(); }

        inline FlowGraph const* GetRootGraph() const { return m_editorGraph.GetRootGraph(); }

        template<typename T>
        TInlineVector<T*, 20> FindAllNodesOfType( VisualGraph::SearchMode mode = VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch typeMatch = VisualGraph::SearchTypeMatch::Exact )
        {
            static_assert( std::is_base_of<GraphNodes::EditorGraphNode, T>::value );
            return GetRootGraph()->FindAllNodesOfType<T>( mode, typeMatch );
        }

        template<typename T>
        TInlineVector<T const*, 20> FindAllNodesOfType( VisualGraph::SearchMode mode = VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch typeMatch = VisualGraph::SearchTypeMatch::Exact ) const
        {
            static_assert( std::is_base_of<GraphNodes::EditorGraphNode, T>::value );
            return GetRootGraph()->FindAllNodesOfType<T const>( mode, typeMatch );
        }

        // Graph information
        inline Category<TypeSystem::TypeInfo const*> const& GetCategorizedNodeTypes() const { return m_categorizedNodeTypes.GetRootCategory(); }

        // Parameters
        //-------------------------------------------------------------------------

        inline int32_t GetNumControlParameters() const { return (int32_t) m_controlParameters.size(); }
        inline int32_t GetNumVirtualParameters() const { return (int32_t) m_virtualParameters.size(); }

        inline TInlineVector<GraphNodes::ControlParameterEditorNode*, 20> const& GetControlParameters() const { return m_controlParameters; }
        inline TInlineVector<GraphNodes::VirtualParameterEditorNode*, 20> const& GetVirtualParameters() const { return m_virtualParameters; }

        GraphNodes::ControlParameterEditorNode* FindControlParameter( UUID parameterID ) const;
        GraphNodes::VirtualParameterEditorNode* FindVirtualParameter( UUID parameterID ) const;

        void CreateControlParameter( GraphValueType type );
        void CreateVirtualParameter( GraphValueType type );

        void RenameControlParameter( UUID parameterID, String const& newName, String const& category );
        void RenameVirtualParameter( UUID parameterID, String const& newName, String const& category );

        void DestroyControlParameter( UUID parameterID );
        void DestroyVirtualParameter( UUID parameterID );

        // Variations and Data Slots
        //-------------------------------------------------------------------------

        inline bool IsValidVariation( StringID variationID ) const { return m_editorGraph.IsValidVariation( variationID ); }
        inline bool IsDefaultVariationSelected() const { return m_selectedVariationID == Variation::s_defaultVariationID; }
        inline StringID GetSelectedVariationID() const { return m_selectedVariationID; }

        // Sets the current selected variation. Assumes a valid variation ID!
        void SetSelectedVariation( StringID variationID );
        
        // Tries to case-insensitively match a supplied variation name to the various variations we have
        void TrySetSelectedVariation( String const& variationName );

        // Called whenever the variation is changed
        inline TEventHandle<> OnVariationSwitched() { return m_onVariationSwitched; }

        Variation const* GetVariation( StringID variationID ) const { return m_editorGraph.GetVariation( variationID ); }
        Variation* GetVariation( StringID variationID ) { return m_editorGraph.GetVariation( variationID ); }

        VariationHierarchy const& GetVariationHierarchy() const { return m_editorGraph.GetVariationHierarchy(); }

        void CreateVariation( StringID variationID, StringID parentVariationID );
        void RenameVariation( StringID existingVariationID, StringID newVariationID );
        void DestroyVariation( StringID variationID );

        // Selection
        //-------------------------------------------------------------------------

        // Return the currently active selection from all the graph editor windows
        TVector<VisualGraph::SelectedNode> const& GetSelectedNodes() const { return m_selectedNodes; }
        void SetSelectedNodes( TVector<VisualGraph::SelectedNode> const& selectedNodes ) { m_selectedNodes = selectedNodes; }
        void ClearSelection() { m_selectedNodes.clear(); }

        // Navigation
        //-------------------------------------------------------------------------

        void NavigateTo( UUID const& nodeID );
        void NavigateTo( VisualGraph::BaseNode* pNode );
        void NavigateTo( VisualGraph::BaseGraph* pGraph );

        inline TEventHandle<VisualGraph::BaseNode*> OnNavigateToNode() { return m_onNavigateToNode; }
        inline TEventHandle<VisualGraph::BaseGraph*> OnNavigateToGraph() { return m_onNavigateToGraph; }

    private:

        void EnsureUniqueParameterName( String& parameterName ) const;

    private:

        ToolsContext const&                                             m_toolsContext;
        EditorGraphDefinition                                           m_editorGraph;
        TVector<TypeSystem::TypeInfo const*>                            m_registeredNodeTypes;
        CategoryTree<TypeSystem::TypeInfo const*>                       m_categorizedNodeTypes;
        TVector<VisualGraph::SelectedNode>                              m_selectedNodes;
        StringID                                                        m_selectedVariationID = Variation::s_defaultVariationID;
        TInlineVector<GraphNodes::ControlParameterEditorNode*, 20>      m_controlParameters;
        TInlineVector<GraphNodes::VirtualParameterEditorNode*, 20>      m_virtualParameters;
        TEvent<VisualGraph::BaseNode*>                                  m_onNavigateToNode;
        TEvent<VisualGraph::BaseGraph*>                                 m_onNavigateToGraph;
        TEvent<>                                                        m_onVariationSwitched;
    };
}