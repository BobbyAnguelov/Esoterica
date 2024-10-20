#include "Animation_ToolsGraph_FlowGraph.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_Parameters.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_DataSlot.h"
#include "EngineTools/Core/CommonToolTypes.h"
#include "EngineTools/Core/CategoryTree.h"
#include "Engine/Animation/AnimationClip.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "EASTL/sort.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_AnimationClip.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_Pose.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    static bool MatchesFilter( TVector<String> const& filterTokens, String string )
    {
        if ( filterTokens.empty() )
        {
            return true;
        }

        if ( string.empty() )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        string.make_lower();
        for ( auto const& token : filterTokens )
        {
            if ( string.find( token ) == String::npos )
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------

        return true;
    }

    //-------------------------------------------------------------------------

    static bool IsNodeTypeValidForSelection( FlowGraph* pFlowGraph, CategoryItem<TypeSystem::TypeInfo const*> const& item, TVector<String> const& filterTokens = TVector<String>(), NodeGraph::FlowNode* pSourceNode = nullptr, NodeGraph::Pin* pFilterPin = nullptr )
    {
        // Parameter references are already handled
        if ( item.m_data->IsDerivedFrom( GraphNodes::ParameterReferenceToolsNode::GetStaticTypeID() ) )
        {
            return false;
        }

        // Is this a valid node for this graph
        if ( !pFlowGraph->CanCreateNode( item.m_data ) )
        {
            return false;
        }

        // Check the graphs specific restrictions
        if ( !pFlowGraph->CanCreateNode( item.m_data ) )
        {
            return false;
        }

        auto pDefaultNode = Cast<GraphNodes::FlowToolsNode>( item.m_data->m_pDefaultInstance );

        // Filter based on pin
        if ( pFilterPin != nullptr )
        {
            if ( pSourceNode->IsInputPin( pFilterPin ) )
            {
                if ( !pDefaultNode->HasOutputPin() )
                {
                    return false;
                }

                NodeGraph::Pin const* pOutputPin = pDefaultNode->GetOutputPin( 0 );
                if ( pOutputPin->m_type != pFilterPin->m_type )
                {
                    return false;
                }

                if ( !pSourceNode->IsValidConnection( pFilterPin->m_ID, pDefaultNode, pOutputPin->m_ID ) )
                {
                    return false;
                }
            }
            else // Check all inputs for matching types
            {
                bool foundValidPin = false;
                for ( NodeGraph::Pin const& inputPin : pDefaultNode->GetInputPins() )
                {
                    if ( inputPin.m_type == pFilterPin->m_type && pDefaultNode->IsValidConnection( inputPin.m_ID, pSourceNode, pFilterPin->m_ID ) )
                    {
                        foundValidPin = true;
                        break;
                    }
                }

                if ( !foundValidPin )
                {
                    return false;
                }
            }
        }

        // User text filter
        if ( !MatchesFilter( filterTokens, item.m_name ) )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        return true;
    }

    static void CollectFilteredNodeTypeItems( FlowGraph* pFlowGraph, TVector<String> const& filterTokens, NodeGraph::FlowNode* pSourceNode, NodeGraph::Pin* pFilterPin, Category<TypeSystem::TypeInfo const*> const& category, CategoryTree<TypeSystem::TypeInfo const*>& outItems )
    {
        for ( auto const& childCategory : category.m_childCategories )
        {
            CollectFilteredNodeTypeItems( pFlowGraph, filterTokens, pSourceNode, pFilterPin, childCategory, outItems );
        }

        //-------------------------------------------------------------------------

        bool const hasUserFilter = !filterTokens.empty() || pFilterPin != nullptr;

        for ( auto const& item : category.m_items )
        {
            if ( !IsNodeTypeValidForSelection( pFlowGraph, item, filterTokens, pSourceNode, pFilterPin ) )
            {
                continue;
            }

            if ( hasUserFilter )
            {
                outItems.GetRootCategory().AddItem( item );
            }
            else
            {
                auto pDefaultNode = Cast<GraphNodes::FlowToolsNode const>( item.m_data->m_pDefaultInstance );
                outItems.AddItem( pDefaultNode->GetCategory(), pDefaultNode->GetTypeName(), item.m_data );
            }
        }
    }

    static TypeSystem::TypeInfo const* DrawCategory( Category<TypeSystem::TypeInfo const*> const& category )
    {
        TypeSystem::TypeInfo const* pNodeTypeToCreate = nullptr;

        // Header
        //-------------------------------------------------------------------------

        if ( category.m_depth == 0 )
        {
            ImGui::SeparatorText( category.m_name.c_str() );
        }
        else if ( category.m_depth > 0 )
        {
            if ( !ImGui::BeginMenu( category.m_name.c_str() ) )
            {
                return pNodeTypeToCreate;
            }
        }

        // Contents
        //-------------------------------------------------------------------------

        for ( auto const& childCategory : category.m_childCategories )
        {
            auto pSelectedNodeToCreate = DrawCategory( childCategory );
            if ( pSelectedNodeToCreate != nullptr )
            {
                pNodeTypeToCreate = pSelectedNodeToCreate;
            }
        }

        if ( pNodeTypeToCreate == nullptr )
        {
            for ( auto const& item : category.m_items )
            {
                auto pDefaultNode = Cast<GraphNodes::FlowToolsNode>( item.m_data->m_pDefaultInstance );
                ImGui::PushStyleColor( ImGuiCol_Text, (ImVec4) pDefaultNode->GetTitleBarColor() );
                ImGui::Bullet();
                ImGui::PopStyleColor();

                ImGui::SameLine();

                bool isMenuItemTriggered = ImGui::MenuItem( item.m_name.c_str() );
                if ( isMenuItemTriggered || ( ImGui::IsItemFocused() && ImGui::IsKeyReleased( ImGuiKey_Enter ) ) )
                {
                    if ( pNodeTypeToCreate == nullptr )
                    {
                        pNodeTypeToCreate = item.m_data;
                    }
                }
            }
        }

        // Footer
        //-------------------------------------------------------------------------

        if ( category.m_depth > 0 )
        {
            ImGui::EndMenu();
        }

        //-------------------------------------------------------------------------

        return pNodeTypeToCreate;
    }

    static TypeSystem::TypeInfo const* DrawNodeTypeCategoryContextMenu( FlowGraph* pFlowGraph, ImVec2 const& mouseCanvasPos, TVector<String> const& filterTokens, NodeGraph::FlowNode* pSourceNode, NodeGraph::Pin* pFilterPin, Category<TypeSystem::TypeInfo const*> const& rootCategory )
    {
        EE_ASSERT( pFlowGraph != nullptr );

        // Collect all relevant items
        //-------------------------------------------------------------------------

        CategoryTree<TypeSystem::TypeInfo const*> contextMenuItems;
        CollectFilteredNodeTypeItems( pFlowGraph, filterTokens, pSourceNode, pFilterPin, rootCategory, contextMenuItems );
        Category<TypeSystem::TypeInfo const*> const& rootCategoryToDraw = contextMenuItems.GetRootCategory();

        // Draw
        //-------------------------------------------------------------------------

        if ( !filterTokens.empty() || pFilterPin != nullptr )
        {
            ImGui::SeparatorText( "Filtered Nodes" );
        }

        if ( contextMenuItems.GetRootCategory().IsEmpty() )
        {
            ImGui::Text( "Nothing Found" );
        }

        TypeSystem::TypeInfo const* pNodeTypeToCreate = DrawCategory( contextMenuItems.GetRootCategory() );
        return pNodeTypeToCreate;
    }

    //-------------------------------------------------------------------------

    FlowGraph::FlowGraph( GraphType type )
        : m_type( type )
    {}

    bool FlowGraph::CanCreateNode( TypeSystem::TypeInfo const* pNodeTypeInfo ) const
    {
        if ( pNodeTypeInfo->IsDerivedFrom( NodeGraph::CommentNode::GetStaticTypeID() ) )
        {
            return true;
        }

        if ( !pNodeTypeInfo->IsDerivedFrom( GraphNodes::FlowToolsNode::s_pTypeInfo->m_ID ) )
        {
            return false;
        }

        // This occurs only on default instance creation
        if ( pNodeTypeInfo->GetDefaultInstance() == nullptr )
        {
            return true;
        }

        auto pFlowNode = Cast<GraphNodes::FlowToolsNode>( pNodeTypeInfo->GetDefaultInstance() );
        return pFlowNode->GetAllowedParentGraphTypes().AreAnyFlagsSet( m_type );
    }

    bool FlowGraph::DrawContextMenuOptions( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos, TVector<String> const& filterTokens, NodeGraph::FlowNode* pSourceNode, NodeGraph::Pin* pOriginPin )
    {
        if ( ctx.m_isReadOnly )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        auto pToolsGraphContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        bool const hasAdvancedFilter = !filterTokens.empty() || pOriginPin != nullptr;

        TypeSystem::TypeInfo const* pNodeTypeToCreate = nullptr;
        GraphNodes::ParameterBaseToolsNode* pParameterToReference = nullptr;

        // Parameters
        //-------------------------------------------------------------------------

        TInlineVector<GraphNodes::ParameterBaseToolsNode*, 20> parameters = pToolsGraphContext->GetParameters();

        // Filter parameters based on origin pin
        for ( int32_t i = 0; i < parameters.size(); i++ )
        {
            bool excludeParameter = false;

            // Filter by name
            if ( !MatchesFilter( filterTokens, parameters[i]->GetName() ) )
            {
                excludeParameter = true;
            }

            // Filter by pin type
            if ( pOriginPin != nullptr && ( pSourceNode->IsOutputPin( pOriginPin ) || GraphNodes::FlowToolsNode::GetValueTypeForPinType( pOriginPin->m_type ) != parameters[i]->GetOutputValueType() ) )
            {
                excludeParameter = true;
            }

            // Remove parameter from the list
            if ( excludeParameter )
            {
                parameters.erase( parameters.begin() + i );
                i--;
            }
        }

        // Sort parameters by name
        auto SortPredicate = [] ( GraphNodes::ParameterBaseToolsNode* const& pA, GraphNodes::ParameterBaseToolsNode* const& pB )
        {
            return _stricmp( pA->GetName(), pB->GetName() ) < 0;
        };

        eastl::sort( parameters.begin(), parameters.end(), SortPredicate );

        // Draw parameter options
        if ( !parameters.empty() )
        {
            ImGui::SeparatorText( "Parameters" );

            bool isUsingSubmenu = false;
            bool shouldDrawSubMenuItems = hasAdvancedFilter;

            if ( !hasAdvancedFilter )
            {
                isUsingSubmenu = shouldDrawSubMenuItems = ImGui::BeginMenu( "Parameters" );
            }

            if ( shouldDrawSubMenuItems )
            {
                if ( parameters.empty() )
                {
                    if ( isUsingSubmenu )
                    {
                        ImGui::Text( "No Parameters" );
                    }
                }
                else
                {
                    for ( auto pParameter : parameters )
                    {
                        ImGui::PushStyleColor( ImGuiCol_Text, (ImVec4) pParameter->GetTitleBarColor() );
                        ImGui::Text( IsOfType<GraphNodes::ControlParameterToolsNode>( pParameter ) ? EE_ICON_ALPHA_C_BOX : EE_ICON_ALPHA_V_CIRCLE );
                        ImGui::PopStyleColor();

                        ImGui::SameLine();

                        bool const isMenuItemTriggered = ImGui::MenuItem( pParameter->GetName() );
                        if ( isMenuItemTriggered || ( ImGui::IsItemFocused() && ImGui::IsKeyReleased( ImGuiKey_Enter ) ) )
                        {
                            pNodeTypeToCreate = GraphNodes::ParameterReferenceToolsNode::s_pTypeInfo;
                            pParameterToReference = pParameter;
                        }
                    }
                }
            }

            if ( isUsingSubmenu )
            {
                ImGui::EndMenu();
            }
        }

        // Draw the node categories
        //-------------------------------------------------------------------------

        if ( pNodeTypeToCreate == nullptr )
        {
            pNodeTypeToCreate = DrawNodeTypeCategoryContextMenu( this, mouseCanvasPos, filterTokens, pSourceNode, pOriginPin, pToolsGraphContext->GetCategorizedNodeTypes() );
        }

        // Create a selected node
        //-------------------------------------------------------------------------

        if ( pNodeTypeToCreate != nullptr )
        {
            NodeGraph::ScopedGraphModification sgm( this );

            // Create target node
            NodeGraph::FlowNode* pCreatedNode = nullptr;
            if ( pParameterToReference )
            {
                pCreatedNode = GraphNodes::ParameterReferenceToolsNode::Create( this, pParameterToReference );
            }
            else
            {
                pCreatedNode = (NodeGraph::FlowNode*) CreateNode( pNodeTypeToCreate );
            }

            // Set node position
            pCreatedNode->SetPosition( mouseCanvasPos );

            // Try to auto-connect nodes
            if ( pSourceNode != nullptr )
            {
                NodeGraph::Pin* pTargetPin = nullptr;

                if ( pSourceNode->IsInputPin( pOriginPin ) )
                {
                    for ( auto const& outputPin : pCreatedNode->GetOutputPins() )
                    {
                        if ( outputPin.m_type == pOriginPin->m_type )
                        {
                            pTargetPin = const_cast<NodeGraph::Pin*>( &outputPin );
                            break;
                        }
                    }

                    if ( pTargetPin != nullptr )
                    {
                        TryMakeConnection( pCreatedNode, pTargetPin, pSourceNode, pOriginPin );
                    }
                }
                else // Output pin
                {
                    for ( auto const& inputPin : pCreatedNode->GetInputPins() )
                    {
                        if ( inputPin.m_type == pOriginPin->m_type )
                        {
                            pTargetPin = const_cast<NodeGraph::Pin*>( &inputPin );
                            break;
                        }
                    }

                    if ( pTargetPin != nullptr )
                    {
                        TryMakeConnection( pSourceNode, pOriginPin, pCreatedNode, pTargetPin );
                    }
                }
            }

            return true;
        }

        return false;
    }

    void FlowGraph::GetSupportedDragAndDropPayloadIDs( TInlineVector<char const*, 10>& outIDs ) const
    {
        outIDs.emplace_back( DragAndDrop::s_filePayloadID );
        outIDs.emplace_back( s_graphParameterPayloadID );
    }

    bool FlowGraph::HandleDragAndDrop( NodeGraph::UserContext* pUserContext, NodeGraph::DragAndDropState const& dragAndDropState )
    {
        auto pToolsGraphContext = static_cast<ToolsGraphUserContext*>( pUserContext );

        // Handle dropped resources
        //-------------------------------------------------------------------------

        if ( dragAndDropState.m_payloadID == DragAndDrop::s_filePayloadID )
        {
            ResourceID const resourceID( (char*) dragAndDropState.m_payloadData.data() );
            if ( !resourceID.IsValid() )
            {
                return true;
            }

            //-------------------------------------------------------------------------

            if ( resourceID.GetResourceTypeID() == AnimationClip::GetStaticResourceTypeID() )
            {
                constexpr static char const* const popupID = "ResourceDropMenu";
                if ( !ImGui::IsPopupOpen( popupID ) )
                {
                    ImGui::OpenPopup( popupID );
                }

                bool actionComplete = false;
                ImGui::SetNextWindowPos( dragAndDropState.m_mouseScreenPos );
                if ( ImGui::BeginPopup( popupID ) )
                {
                    if ( ImGui::MenuItem( "Drop As Clip" ) )
                    {
                        NodeGraph::ScopedGraphModification sgm( this );
                        auto pDataSlotNode = CreateNode<GraphNodes::AnimationClipToolsNode>();
                        pDataSlotNode->SetPosition( dragAndDropState.m_mouseCanvasPos );
                        pDataSlotNode->Rename( resourceID.GetFilenameWithoutExtension() );
                        pDataSlotNode->SetVariationResourceID( resourceID );
                        actionComplete = true;
                        ImGui::CloseCurrentPopup();
                    }

                    if ( ImGui::MenuItem( "Drop As Pose" ) )
                    {
                        NodeGraph::ScopedGraphModification sgm( this );
                        auto pDataSlotNode = CreateNode<GraphNodes::AnimationPoseToolsNode>();
                        pDataSlotNode->SetPosition( dragAndDropState.m_mouseCanvasPos );
                        pDataSlotNode->Rename( resourceID.GetFilenameWithoutExtension() );
                        pDataSlotNode->SetVariationResourceID( resourceID );
                        actionComplete = true;
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }

                return actionComplete;
            }
            else // Handle other resource types
            {
                ResourceTypeID const resourceTypeID = resourceID.GetResourceTypeID();

                // Try to find a matching slot node type
                TypeSystem::TypeInfo const* pFoundNodeTypeInfo = nullptr;
                auto const dataSlotTypeInfos = pToolsGraphContext->m_pTypeRegistry->GetAllDerivedTypes( GraphNodes::DataSlotToolsNode::GetStaticTypeID(), false, false, true );
                for ( auto pTypeInfo : dataSlotTypeInfos )
                {
                    auto pDefaultSlotNodeInstance = Cast<GraphNodes::DataSlotToolsNode>( pTypeInfo->m_pDefaultInstance );
                    if ( pDefaultSlotNodeInstance->GetSlotResourceTypeID() == resourceTypeID )
                    {
                        pFoundNodeTypeInfo = pTypeInfo;
                        break;
                    }
                }

                if ( pFoundNodeTypeInfo == nullptr )
                {
                    return true;
                }

                //-------------------------------------------------------------------------

                NodeGraph::ScopedGraphModification sgm( this );
                auto pDataSlotNode = Cast<GraphNodes::DataSlotToolsNode>( CreateNode( pFoundNodeTypeInfo, dragAndDropState.m_mouseCanvasPos ) );
                pDataSlotNode->Rename( resourceID.GetFilenameWithoutExtension() );
                pDataSlotNode->SetVariationResourceID( resourceID );
            }
        }

        // Handle dropped parameters
        //-------------------------------------------------------------------------

        if ( dragAndDropState.m_payloadID == s_graphParameterPayloadID )
        {
            char* pPayloadStr = (char*) dragAndDropState.m_payloadData.data();

            for ( auto pControlParameter : pToolsGraphContext->GetParameters() )
            {
                if ( pControlParameter->GetParameterName().comparei( pPayloadStr ) == 0 )
                {
                    NodeGraph::ScopedGraphModification sgm( this );
                    auto pNode = GraphNodes::ParameterReferenceToolsNode::Create( this, pControlParameter );
                    pNode->SetPosition( dragAndDropState.m_mouseCanvasPos );
                    break;
                }
            }
        }

        //-------------------------------------------------------------------------

        return true;
    }
}