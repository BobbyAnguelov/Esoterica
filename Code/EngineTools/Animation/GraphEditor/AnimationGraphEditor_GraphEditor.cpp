#include "AnimationGraphEditor_GraphEditor.h"
#include "AnimationGraphEditor_Context.h"
#include "EditorGraph/Nodes/Animation_EditorGraphNode_AnimationClip.h"
#include "EditorGraph/Nodes/Animation_EditorGraphNode_Ragdoll.h"
#include "EditorGraph/Animation_EditorGraph_Definition.h"
#include "EngineTools/Resource/ResourceDatabase.h"
#include "Engine/Physics/PhysicsRagdoll.h"
#include "Engine/UpdateContext.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    using namespace EE::Animation::GraphNodes;

    //-------------------------------------------------------------------------

    void GraphEditor::GraphView::DrawContextMenuForGraph()
    {
        EE_ASSERT( m_pGraph != nullptr );

        VisualGraph::GraphView::DrawContextMenuForGraph();

        //-------------------------------------------------------------------------

        ImGui::Separator();

        if ( IsViewingStateMachineGraph() )
        {
            EE_ASSERT( m_pGraph != nullptr );
            auto pStateMachineEditorGraph = static_cast<StateMachineGraph*>( m_pGraph );

            if ( ImGui::MenuItem( "Blend Tree State" ) )
            {
                pStateMachineEditorGraph->CreateNewState( m_contextMenuState.m_mouseCanvasPos );
            }

            if ( ImGui::MenuItem( "Off State" ) )
            {
                pStateMachineEditorGraph->CreateNewOffState( m_contextMenuState.m_mouseCanvasPos );
            }
        }
        else // Flow Graph
        {
            auto& editorContext = m_graphEditor.m_editorContext;
            auto pFlowEditorGraph = static_cast<FlowGraph*>( m_pGraph );

            //-------------------------------------------------------------------------

            if ( ImGui::BeginMenu( "Control Parameters" ) )
            {
                TInlineVector<ControlParameterEditorNode*, 20> sortedControlParameters = editorContext.GetControlParameters();
                eastl::sort( sortedControlParameters.begin(), sortedControlParameters.end(), [] ( ControlParameterEditorNode* const& pA, ControlParameterEditorNode* const& pB ) { return strcmp( pA->GetName(), pB->GetName() ) < 0; } );

                TInlineVector<VirtualParameterEditorNode*, 20> sortedVirtualParameters = editorContext.GetVirtualParameters();
                eastl::sort( sortedVirtualParameters.begin(), sortedVirtualParameters.end(), [] ( VirtualParameterEditorNode* const& pA, VirtualParameterEditorNode* const& pB ) { return strcmp( pA->GetName(), pB->GetName() ) < 0; } );

                //-------------------------------------------------------------------------

                if ( editorContext.GetControlParameters().empty() && editorContext.GetVirtualParameters().empty() )
                {
                    ImGui::Text( "No Parameters" );
                }
                else
                {
                    for ( auto pParameter : sortedControlParameters )
                    {
                        ImGui::PushStyleColor( ImGuiCol_Text, (ImVec4) pParameter->GetTitleBarColor() );
                        if ( ImGui::MenuItem( pParameter->GetName() ) )
                        {
                            VisualGraph::ScopedGraphModification sgm( m_pGraph );
                            auto pNode = pFlowEditorGraph->CreateNode<ParameterReferenceEditorNode>( pParameter );
                            pNode->SetCanvasPosition( m_contextMenuState.m_mouseCanvasPos );
                        }
                        ImGui::PopStyleColor();
                    }

                    if ( !sortedVirtualParameters.empty() )
                    {
                        ImGui::Separator();
                    }

                    for ( auto pParameter : sortedVirtualParameters )
                    {
                        ImGui::PushStyleColor( ImGuiCol_Text, (ImVec4) pParameter->GetTitleBarColor() );
                        if ( ImGui::MenuItem( pParameter->GetName() ) )
                        {
                            VisualGraph::ScopedGraphModification sgm( m_pGraph );
                            auto pNode = pFlowEditorGraph->CreateNode<ParameterReferenceEditorNode>( pParameter );
                            pNode->SetCanvasPosition( m_contextMenuState.m_mouseCanvasPos );
                        }
                        ImGui::PopStyleColor();
                    }
                }

                ImGui::EndMenu();
            }

            //-------------------------------------------------------------------------

            DrawNodeTypeCategoryContextMenu( m_contextMenuState.m_mouseCanvasPos, pFlowEditorGraph, editorContext.GetCategorizedNodeTypes() );
        }
    }

    void GraphEditor::GraphView::DrawContextMenuForNode()
    {
        if ( IsViewingStateMachineGraph() )
        {
            auto pStateNode = TryCast<VisualGraph::SM::State>( m_contextMenuState.m_pNode );
            if ( pStateNode != nullptr )
            {
                if ( ImGui::MenuItem( "Make Default Entry State" ) )
                {
                    GetStateMachineGraph()->SetDefaultEntryState(pStateNode->GetID());
                }
            }
        }
    }

    bool GraphEditor::GraphView::DrawNodeTypeCategoryContextMenu( ImVec2 const& mouseCanvasPos, FlowGraph* pGraph, Category<TypeSystem::TypeInfo const*> const& category )
    {
        EE_ASSERT( m_pGraph != nullptr );
        EE_ASSERT( IsViewingFlowGraph() );

        auto NodeFilter = [this, pGraph] ( CategoryItem<TypeSystem::TypeInfo const*> const& item )
        {
            // Parameter references are already handled
            if ( item.m_data->m_ID == ParameterReferenceEditorNode::GetStaticTypeID() )
            {
                return false;
            }

            // Is this a valid node for this graph
            if ( !m_pGraph->CanCreateNode( item.m_data ) )
            {
                return false;
            }

            // Check the graphs specific restrictions
            auto pDefaultNode = Cast<EditorGraphNode>( item.m_data->m_pDefaultInstance );
            if ( !pDefaultNode->GetAllowedParentGraphTypes().AreAnyFlagsSet( pGraph->GetType() ) )
            {
                return false;
            }

            return true;
        };

        if ( !category.HasItemsThatMatchFilter( NodeFilter ) )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        auto DrawItems = [this, &NodeFilter, &category, &mouseCanvasPos, &pGraph] ()
        {
            for ( auto const& childCategory : category.m_childCategories )
            {
                DrawNodeTypeCategoryContextMenu( mouseCanvasPos, pGraph, childCategory );
            }

            if ( category.m_depth == -1 )
            {
                ImGui::Separator();
            }

            for ( auto const& item : category.m_items )
            {
                if ( !NodeFilter( item ) )
                {
                    continue;
                }

                if ( ImGui::MenuItem( item.m_name.c_str() ) )
                {
                    VisualGraph::ScopedGraphModification sgm( m_pGraph );
                    auto pEditorGraph = static_cast<FlowGraph*>( m_pGraph );
                    auto pNode = pEditorGraph->CreateNode( item.m_data );
                    pNode->SetCanvasPosition( mouseCanvasPos );
                }
            }
        };

        if ( category.m_depth == -1 )
        {
            DrawItems();
        }
        else if ( category.m_depth == 0 )
        {
            ImGuiX::TextSeparator( category.m_name.c_str() );
            DrawItems();
        }
        else
        {
            if ( ImGui::BeginMenu( category.m_name.c_str() ) )
            {
                DrawItems();
                ImGui::EndMenu();
            }
        }

        return true;
    }

    void GraphEditor::GraphView::OnGraphDoubleClick( VisualGraph::BaseGraph* pGraph )
    {
        EE_ASSERT( pGraph != nullptr );

        auto pParentNode = pGraph->GetParentNode();
        if ( pParentNode != nullptr )
        {
            m_graphEditor.NavigateTo( pParentNode->GetParentGraph() );
        }
    }

    void GraphEditor::GraphView::OnNodeDoubleClick( VisualGraph::BaseNode* pNode )
    {
        EE_ASSERT( pNode != nullptr );

        if ( pNode->HasChildGraph() )
        {
            m_graphEditor.NavigateTo( pNode->GetChildGraph() );
        }
        else if ( auto pParameterReference = TryCast<ParameterReferenceEditorNode>( pNode ) )
        {
            if ( auto pVP = pParameterReference->GetReferencedVirtualParameter() )
            {
                m_graphEditor.NavigateTo( pVP->GetChildGraph() );
            }
        }
        else if ( auto pDataSlotNode = TryCast<DataSlotEditorNode>( pNode ) )
        {
            ResourceID const resourceID = pDataSlotNode->GetResourceID( m_graphEditor.m_editorContext.GetVariationHierarchy(), m_graphEditor.m_editorContext.GetSelectedVariationID() );
            if ( resourceID.IsValid() )
            {
                m_graphEditor.m_resourceViewRequests.emplace_back( resourceID );
            }
        }
    }

    void GraphEditor::GraphView::DrawExtraInformation( VisualGraph::DrawContext const& ctx )
    {
        if ( IsViewingStateMachineGraph() )
        {
            auto pEditorGraph = static_cast<StateMachineGraph*>( m_pGraph );

            auto const stateNodes = pEditorGraph->FindAllNodesOfType<StateBaseEditorNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );
            for ( auto pStateNode : stateNodes )
            {
                ImRect const nodeRect = GetNodeWindowRect( pStateNode );
                ImVec2 const iconSize = ImGui::CalcTextSize( EE_ICON_ASTERISK );
                ImVec2 iconOffset( 0, iconSize.y + 4.0f );

                // Draw entry state marker
                if ( pStateNode->GetID() == pEditorGraph->GetDefaultEntryStateID() )
                {
                    ctx.m_pDrawList->AddText( nodeRect.Min + ctx.m_windowRect.Min - iconOffset, ImGuiX::ConvertColor( Colors::LimeGreen ), EE_ICON_ARROW_DOWN_CIRCLE );
                    iconOffset.x -= iconSize.x + 4.0f;
                }

                // Draw global transition marker
                if ( pEditorGraph->HasGlobalTransitionForState( pStateNode->GetID() ) )
                {
                    ctx.m_pDrawList->AddText( nodeRect.Min + ctx.m_windowRect.Min - iconOffset, ImGuiX::ConvertColor( Colors::OrangeRed ), EE_ICON_LIGHTNING_BOLT_CIRCLE );
                }
            }
        }
    }

    void GraphEditor::GraphView::HandleDragAndDrop( ImVec2 const& mouseCanvasPos )
    {
        if ( m_pGraph == nullptr )
        {
            return;
        }

        auto& editorContext = m_graphEditor.m_editorContext;
        auto const* pToolsContext = editorContext.GetToolsContext();

        // Handle dropped resources
        //-------------------------------------------------------------------------

        if ( ImGuiPayload const* payload = ImGui::AcceptDragDropPayload( "ResourceFile", ImGuiDragDropFlags_AcceptBeforeDelivery ) )
        {
            if ( IsViewingStateMachineGraph() )
            {
                return;
            }

            InlineString payloadStr = (char*) payload->Data;

            ResourceID const resourceID( payloadStr.c_str() );
            if ( !resourceID.IsValid() || !pToolsContext->m_pResourceDatabase->DoesResourceExist(resourceID) )
            {
                return;
            }

            bool const isAnimationClipResource = ( resourceID.GetResourceTypeID() == AnimationClip::GetStaticResourceTypeID() );
            bool const isRagdollResource = ( resourceID.GetResourceTypeID() == Physics::RagdollDefinition::GetStaticResourceTypeID() );

            if ( !isAnimationClipResource && !isRagdollResource )
            {
                return;
            }

            if ( !payload->IsDelivery() )
            {
                return;
            }

            auto pGraphDefinition = editorContext.GetGraphDefinition();
            auto pFlowEditorGraph = static_cast<FlowGraph*>( m_pGraph );

            if( isAnimationClipResource )
            {
                VisualGraph::ScopedGraphModification sgm( m_pGraph );
                auto pNode = pFlowEditorGraph->CreateNode<AnimationClipEditorNode>();
                pNode->SetName( resourceID.GetFileNameWithoutExtension() );
                pNode->SetDefaultResourceID( resourceID );
                pNode->SetCanvasPosition( mouseCanvasPos );
                return;
            }
            else if ( isRagdollResource )
            {
                VisualGraph::ScopedGraphModification sgm( m_pGraph );
                auto pNode = pFlowEditorGraph->CreateNode<PoweredRagdollEditorNode>();
                pNode->SetName( resourceID.GetFileNameWithoutExtension() );
                pNode->SetDefaultResourceID( resourceID );
                pNode->SetCanvasPosition( mouseCanvasPos );
                return;
            }
        }

        // Handle dropped parameters
        //-------------------------------------------------------------------------

        if ( ImGuiPayload const* payload = ImGui::AcceptDragDropPayload( "AnimGraphParameter", ImGuiDragDropFlags_AcceptBeforeDelivery ) )
        {
            if ( IsViewingStateMachineGraph() )
            {
                return;
            }

            if ( !payload->IsDelivery() )
            {
                return;
            }

            InlineString payloadStr = (char*) payload->Data;

            auto pGraphDefinition = editorContext.GetGraphDefinition();
            auto pFlowEditorGraph = static_cast<FlowGraph*>( m_pGraph );

            for ( auto pControlParameter : editorContext.GetControlParameters() )
            {
                if ( payloadStr == pControlParameter->GetName() )
                {
                    VisualGraph::ScopedGraphModification sgm( m_pGraph );
                    auto pNode = pFlowEditorGraph->CreateNode<ParameterReferenceEditorNode>( pControlParameter );
                    pNode->SetCanvasPosition( mouseCanvasPos );
                    return;
                }
            }

            for ( auto pVirtualParameter : editorContext.GetVirtualParameters() )
            {
                if ( payloadStr == pVirtualParameter->GetName() )
                {
                    VisualGraph::ScopedGraphModification sgm( m_pGraph );
                    auto pNode = pFlowEditorGraph->CreateNode<ParameterReferenceEditorNode>( pVirtualParameter );
                    pNode->SetCanvasPosition( mouseCanvasPos );
                    return;
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    GraphEditor::GraphEditor(GraphEditorContext& editorContext )
        : m_editorContext( editorContext )
        , m_primaryGraphView( *this )
        , m_secondaryGraphView( *this )
    {}

    void GraphEditor::OnUndoRedo()
    {
        auto pFoundGraph = m_editorContext.GetRootGraph()->FindPrimaryGraph( m_primaryViewGraphID );
        if ( pFoundGraph != nullptr )
        {
            if ( m_primaryGraphView.GetViewedGraph() != pFoundGraph )
            {
                m_primaryGraphView.SetGraphToView( pFoundGraph );
            }

            UpdateSecondaryViewState();
        }
        else
        {
            NavigateTo( m_editorContext.GetRootGraph() );
        }
    }

    void GraphEditor::NavigateTo( VisualGraph::BaseNode* pNode, bool focusViewOnNode )
    {
        EE_ASSERT( pNode != nullptr );

        // Navigate to the appropriate graph
        auto pParentGraph = pNode->GetParentGraph();
        EE_ASSERT( pParentGraph != nullptr );
        NavigateTo( pParentGraph );

        // Select node
        if ( m_primaryGraphView.GetViewedGraph()->FindNode( pNode->GetID() ) )
        {
            m_primaryGraphView.SelectNode( pNode );
            if( focusViewOnNode )
            {
                m_primaryGraphView.CenterView( pNode );
            }
        }
        else if ( m_secondaryGraphView.GetViewedGraph() != nullptr && m_secondaryGraphView.GetViewedGraph()->FindNode( pNode->GetID() ) )
        {
            m_secondaryGraphView.SelectNode( pNode );
            if ( focusViewOnNode )
            {
                m_secondaryGraphView.CenterView( pNode );
            }
        }
    }

    void GraphEditor::NavigateTo( VisualGraph::BaseGraph* pGraph )
    {
        // If the graph we wish to navigate to is a secondary graph, we need to set the primary view and selection accordingly
        auto pParentNode = pGraph->GetParentNode();
        if ( pParentNode != nullptr && pParentNode->GetSecondaryGraph() == pGraph )
        {
            NavigateTo( pParentNode->GetParentGraph() );
            m_primaryGraphView.SelectNode( pParentNode );
            UpdateSecondaryViewState();
        }
        else // Directly update the primary view
        {
            if ( m_primaryGraphView.GetViewedGraph() != pGraph )
            {
                m_primaryGraphView.SetGraphToView( pGraph );
                m_primaryViewGraphID = pGraph->GetID();
            }
        }
    }

    void GraphEditor::UpdateSecondaryViewState()
    {
        VisualGraph::BaseGraph* pSecondaryGraphToView = nullptr;

        if ( m_primaryGraphView.HasSelectedNodes() )
        {
            auto pSelectedNode = m_primaryGraphView.GetSelectedNodes().back().m_pNode;
            if ( pSelectedNode->HasSecondaryGraph() )
            {
                if ( auto pSecondaryGraph = TryCast<VisualGraph::FlowGraph>( pSelectedNode->GetSecondaryGraph() ) )
                {
                    pSecondaryGraphToView = pSecondaryGraph;
                }
            }
            else if( auto pParameterReference = TryCast<ParameterReferenceEditorNode>( pSelectedNode ) )
            {
                if ( auto pVP = pParameterReference->GetReferencedVirtualParameter() )
                {
                    pSecondaryGraphToView = pVP->GetChildGraph();
                }
            }
        }

        if ( m_secondaryGraphView.GetViewedGraph() != pSecondaryGraphToView )
        {
            m_secondaryGraphView.SetGraphToView( pSecondaryGraphToView );
        }
    }

    void GraphEditor::UpdateAndDraw( UpdateContext const& context, GraphNodeContext* pGraphNodeContext, ImGuiWindowClass* pWindowClass, char const* pWindowName )
    {
        m_resourceViewRequests.clear();
        bool isGraphViewFocused = false;

        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );
        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( pWindowName ) )
        {
            auto pTypeRegistry = context.GetSystem<TypeSystem::TypeRegistry>();

            isGraphViewFocused = ImGui::IsWindowFocused( ImGuiFocusedFlags_ChildWindows );

            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 0 ) );

            // Navigation Bar
            //-------------------------------------------------------------------------

            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4, 2 ) );
            ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 1, 1 ) );
            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 4, 1 ) );
            if( ImGui::BeginChild( "NavBar", ImVec2( ImGui::GetContentRegionAvail().x, 24 ), false, ImGuiWindowFlags_AlwaysUseWindowPadding ) )
            {
                ImGuiX::ScopedFont const sf( ImGuiX::Font::SmallBold );
                ImVec2 const navBarDimensions = ImGui::GetContentRegionAvail();

                if ( ImGuiX::ColoredButton( Colors::Green, Colors::White, EE_ICON_HOME"##GoHome", ImVec2( 22, -1 ) ) )
                {
                    NavigateTo( m_editorContext.GetRootGraph() );
                }
                ImGuiX::ItemTooltip( "Go to root graph" );

                //-------------------------------------------------------------------------

                TInlineVector<VisualGraph::BaseGraph*,10> pathToRoot;

                auto pGraph = m_primaryGraphView.GetViewedGraph();
                while ( pGraph != nullptr && pGraph != m_editorContext.GetRootGraph() )
                {
                    pathToRoot.emplace_back( pGraph );
                    if ( pGraph->GetParentNode() != nullptr )
                    {
                        pGraph = pGraph->GetParentNode()->GetParentGraph();
                    }
                    else
                    {
                        pGraph = nullptr;
                    }
                }

                ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0, 0, 0, 0 ) );
                for ( auto i = (int32_t) pathToRoot.size() - 1; i >= 0; i-- )
                {
                    ImGui::SameLine( 0, -1 );
                    ImGui::Text( "/" );

                    InlineString const str( InlineString::CtorSprintf(), "%s##%s", pathToRoot[i]->GetTitle(), pathToRoot[i]->GetID().ToString().c_str() );
                    ImGui::SameLine( 0, -1 );
                    if ( ImGui::Button( str.c_str() ) )
                    {
                        NavigateTo( pathToRoot[i] );
                    }
                }
                ImGui::PopStyleColor();

                //-------------------------------------------------------------------------

                if ( m_primaryGraphView.IsViewingStateMachineGraph() )
                {
                    constexpr static float const buttonWidth = 64;

                    ImGui::SameLine( navBarDimensions.x - buttonWidth * 2, 0 );
                    ImGui::AlignTextToFramePadding();
                    if ( ImGuiX::ColoredButton( Colors::Green, Colors::White, EE_ICON_DOOR_OPEN" Entry", ImVec2( buttonWidth, -1 ) ) )
                    {
                        auto pSM = Cast<StateMachineGraph>( m_primaryGraphView.GetViewedGraph() );
                        NavigateTo( pSM->GetEntryStateOverrideConduit() );
                    }
                    ImGuiX::ItemTooltip( "Entry State Overrides" );

                    ImGui::SameLine( 0, -1 );
                    if ( ImGuiX::ColoredButton( Colors::OrangeRed, Colors::White, EE_ICON_LIGHTNING_BOLT"Global", ImVec2( buttonWidth, -1 ) ) )
                    {
                        auto pSM = Cast<StateMachineGraph>( m_primaryGraphView.GetViewedGraph() );
                        NavigateTo( pSM->GetGlobalTransitionConduit() );
                    }
                    ImGuiX::ItemTooltip( "Global Transitions" );

                    ImGui::SameLine();
                }
                else
                {
                    ImGui::SameLine( navBarDimensions.x - 14, 0 );
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleVar( 3 );

            // Primary View
            //-------------------------------------------------------------------------

            m_primaryGraphView.UpdateAndDraw( *pTypeRegistry, m_primaryGraphViewHeight, pGraphNodeContext );

            if ( m_primaryGraphView.HasSelectionChangedThisFrame() )
            {
                m_editorContext.SetSelectedNodes( m_primaryGraphView.GetSelectedNodes() );
            }

            // Splitter
            //-------------------------------------------------------------------------
            
            ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 0.0f );
            ImGui::Button( "##GraphViewSplitter", ImVec2( -1, 3 ) );
            ImGui::PopStyleVar();

            if ( ImGui::IsItemHovered() )
            {
                ImGui::SetMouseCursor( ImGuiMouseCursor_ResizeNS );
            }

            if ( ImGui::IsItemActive() )
            {
                m_primaryGraphViewHeight += ImGui::GetIO().MouseDelta.y;
                m_primaryGraphViewHeight = Math::Max( 25.0f, m_primaryGraphViewHeight );
            }

            // SecondaryView
            //-------------------------------------------------------------------------

            UpdateSecondaryViewState();
            m_secondaryGraphView.UpdateAndDraw( *pTypeRegistry, 0.0f, pGraphNodeContext );

            if ( m_secondaryGraphView.HasSelectionChangedThisFrame() )
            {
                m_editorContext.SetSelectedNodes( m_secondaryGraphView.GetSelectedNodes() );
            }

            //-------------------------------------------------------------------------

            ImGui::PopStyleVar();
        }
        ImGui::End();
        ImGui::PopStyleVar();

        // Handle Focus and selection 
        //-------------------------------------------------------------------------

        HandleFocusAndSelectionChanges();

        // Process view change requests
        //-------------------------------------------------------------------------

        for ( auto const& resourceToView : m_resourceViewRequests )
        {
            m_editorContext.GetToolsContext()->TryOpenResource( resourceToView );
        }
    }

    void GraphEditor::HandleFocusAndSelectionChanges()
    {
        GraphEditor::GraphView* pCurrentlyFocusedGraphView = nullptr;

        // Get the currently focused view
        //-------------------------------------------------------------------------

        if ( m_primaryGraphView.HasFocus() )
        {
            pCurrentlyFocusedGraphView = &m_primaryGraphView;
        }
        else if ( m_secondaryGraphView.HasFocus() )
        {
            pCurrentlyFocusedGraphView = &m_secondaryGraphView;
        }

        // Has the focus within the graph editor tool changed?
        //-------------------------------------------------------------------------

        if ( pCurrentlyFocusedGraphView != nullptr && pCurrentlyFocusedGraphView != m_pFocusedGraphView )
        {
            m_pFocusedGraphView = pCurrentlyFocusedGraphView;
        }
    }
}