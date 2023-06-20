#include "Animation_ToolsGraph_FlowGraph.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_Parameters.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_DataSlot.h"
#include "EngineTools/Core/CategoryTree.h"
#include "EASTL/sort.h"
#include "System/TypeSystem/TypeRegistry.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    static bool MatchesFilter( TVector<String> const& filterTokens, String string )
    {
        if ( string.empty() )
        {
            return false;
        }

        if ( filterTokens.empty() )
        {
            return true;
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

    static TypeSystem::TypeInfo const* DrawNodeTypeCategoryContextMenu( FlowGraph* pFlowGraph, ImVec2 const& mouseCanvasPos, TVector<String> const& filterTokens, VisualGraph::Flow::Pin* pFilterPin, Category<TypeSystem::TypeInfo const*> const& category )
    {
        EE_ASSERT( pFlowGraph != nullptr );

        bool const hasAdvancedFilter = !filterTokens.empty() || pFilterPin != nullptr;

        //-------------------------------------------------------------------------

        auto NodeFilter = [pFlowGraph, pFilterPin, &filterTokens] ( CategoryItem<TypeSystem::TypeInfo const*> const& item )
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
            auto pDefaultNode = Cast<GraphNodes::FlowToolsNode>( item.m_data->m_pDefaultInstance );
            if ( !pDefaultNode->GetAllowedParentGraphTypes().AreAnyFlagsSet( pFlowGraph->GetType() ) )
            {
                return false;
            }

            // Filter based on pin
            if ( pFilterPin != nullptr )
            {
                if ( pFilterPin->IsInputPin() )
                {
                    if ( !pDefaultNode->HasOutputPin() || pDefaultNode->GetOutputPin( 0 )->m_type != pFilterPin->m_type )
                    {
                        return false;
                    }
                }
                else // Check all inputs for matching types
                {
                    bool foundValidPin = false;
                    for ( auto const& inputPin : pDefaultNode->GetInputPins() )
                    {
                        if ( inputPin.m_type == pFilterPin->m_type )
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

            return true;
        };

        if ( !category.HasItemsThatMatchFilter( NodeFilter ) )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        auto DrawItems = [pFlowGraph, &NodeFilter, &pFilterPin, &category, &mouseCanvasPos, &filterTokens] () -> TypeSystem::TypeInfo const*
        {
            for ( auto const& childCategory : category.m_childCategories )
            {
                auto pNodeTypeToCreate = DrawNodeTypeCategoryContextMenu( pFlowGraph, mouseCanvasPos, filterTokens, pFilterPin, childCategory );
                if ( pNodeTypeToCreate != nullptr )
                {
                    return pNodeTypeToCreate;
                }
            }

            for ( auto const& item : category.m_items )
            {
                if ( !NodeFilter( item ) )
                {
                    continue;
                }

                auto pDefaultNode = Cast<GraphNodes::FlowToolsNode>( item.m_data->m_pDefaultInstance );
                ImGui::PushStyleColor( ImGuiCol_Text, (ImVec4) pDefaultNode->GetTitleBarColor() );
                ImGui::Bullet();
                ImGui::PopStyleColor();

                ImGui::SameLine();

                bool isMenuItemTriggered = ImGui::MenuItem( item.m_name.c_str() );
                if ( isMenuItemTriggered || ( ImGui::IsItemFocused() && ImGui::IsKeyReleased( ImGuiKey_Enter ) ) )
                {
                    return item.m_data;
                }
            }

            return nullptr;
        };

        //-------------------------------------------------------------------------

        TypeSystem::TypeInfo const* pNodeTypeToCreate = nullptr;

        if ( category.m_depth == -1 )
        {
            pNodeTypeToCreate = DrawItems();
        }
        else if ( category.m_depth == 0 )
        {
            ImGuiX::TextSeparator( category.m_name.c_str() );
            pNodeTypeToCreate = DrawItems();
        }
        else
        {
            if ( hasAdvancedFilter )
            {
                pNodeTypeToCreate = DrawItems();
            }
            else
            {
                if ( ImGui::BeginMenu( category.m_name.c_str() ) )
                {
                    pNodeTypeToCreate = DrawItems();
                    ImGui::EndMenu();
                }
            }
        }

        //-------------------------------------------------------------------------

        return pNodeTypeToCreate;
    }

    template<typename T>
    static GraphNodes::ParameterReferenceToolsNode* CreateParameterReference( FlowGraph* pGraph, T* pParameter )
    {
        switch ( pParameter->GetValueType() )
        {
            case GraphValueType::Bool:
            return pGraph->CreateNode<GraphNodes::BoolParameterReferenceToolsNode>( pParameter );
            break;

            case GraphValueType::ID:
            return pGraph->CreateNode<GraphNodes::IDParameterReferenceToolsNode>( pParameter );
            break;

            case GraphValueType::Float:
            return pGraph->CreateNode<GraphNodes::FloatParameterReferenceToolsNode>( pParameter );
            break;

            case GraphValueType::Vector:
            return pGraph->CreateNode<GraphNodes::VectorParameterReferenceToolsNode>( pParameter );
            break;

            case GraphValueType::Target:
            return pGraph->CreateNode<GraphNodes::TargetParameterReferenceToolsNode>( pParameter );
            break;

            case GraphValueType::BoneMask:
            return pGraph->CreateNode<GraphNodes::BoneMaskParameterReferenceToolsNode>( pParameter );
            break;

            default:
            return nullptr;
            break;
        }
    }

    template<typename T>
    static void FilterParameters( TInlineVector<T*, 20>& parameters, TVector<String> const& filterTokens, VisualGraph::Flow::Pin* pFilterPin )
    {
        for ( int32_t i = 0; i < parameters.size(); i++ )
        {
            bool excludeParameter = false;

            // Filter by name
            if ( !MatchesFilter( filterTokens, parameters[i]->GetName() ) )
            {
                excludeParameter = true;
            }

            // Filter by pin type
            if ( pFilterPin != nullptr && ( pFilterPin->IsOutputPin() || GraphNodes::FlowToolsNode::GetValueTypeForPinType( pFilterPin->m_type ) != parameters[i]->GetValueType() ) )
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
    }

    template<typename T>
    static void DrawParameterItems( bool hasAdvancedFilter, char const* pMenuLabel, TInlineVector<T*, 20>&parameters, TypeSystem::TypeInfo const*& pNodeTypeToCreate, T*& pParameterToReference )
    {
        bool isUsingSubmenu = false;
        bool shouldDrawSubMenuItems = hasAdvancedFilter;

        if ( !hasAdvancedFilter )
        {
            isUsingSubmenu = shouldDrawSubMenuItems = ImGui::BeginMenu( pMenuLabel );
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

    //-------------------------------------------------------------------------

    FlowGraph::FlowGraph( GraphType type )
        : m_type( type )
    {}

    void FlowGraph::SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& graphObjectValue )
    {
        VisualGraph::FlowGraph::SerializeCustom( typeRegistry, graphObjectValue );
        m_type = (GraphType) graphObjectValue["GraphType"].GetUint();
    }

    void FlowGraph::SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const
    {
        VisualGraph::FlowGraph::SerializeCustom( typeRegistry, writer );
        writer.Key( "GraphType" );
        writer.Uint( (uint8_t) m_type );
    }

    bool FlowGraph::DrawContextMenuOptions( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos, TVector<String> const& filterTokens, VisualGraph::Flow::Node* pSourceNode, VisualGraph::Flow::Pin* pOriginPin )
    {
        if ( ctx.m_isReadOnly )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        auto pToolsGraphContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        bool const hasAdvancedFilter = !filterTokens.empty() || pOriginPin != nullptr;

        TypeSystem::TypeInfo const* pNodeTypeToCreate = nullptr;
        GraphNodes::ControlParameterToolsNode* pParameterToReference = nullptr;
        GraphNodes::VirtualParameterToolsNode* pVirtualParameterToReference = nullptr;

        // Parameters
        //-------------------------------------------------------------------------

        TInlineVector<GraphNodes::ControlParameterToolsNode*, 20> sortedControlParameters = pToolsGraphContext->GetControlParameters();
        FilterParameters( sortedControlParameters, filterTokens, pOriginPin );
        eastl::sort( sortedControlParameters.begin(), sortedControlParameters.end(), [] ( GraphNodes::ControlParameterToolsNode* const& pA, GraphNodes::ControlParameterToolsNode* const& pB ) { return strcmp( pA->GetName(), pB->GetName() ) < 0; } );

        TInlineVector<GraphNodes::VirtualParameterToolsNode*, 20> sortedVirtualParameters = pToolsGraphContext->GetVirtualParameters();
        FilterParameters( sortedVirtualParameters, filterTokens, pOriginPin );
        eastl::sort( sortedVirtualParameters.begin(), sortedVirtualParameters.end(), [] ( GraphNodes::VirtualParameterToolsNode* const& pA, GraphNodes::VirtualParameterToolsNode* const& pB ) { return strcmp( pA->GetName(), pB->GetName() ) < 0; } );

        if ( ( sortedVirtualParameters.size() + sortedControlParameters.size() ) > 0 )
        {
            ImGuiX::TextSeparator( "Parameters" );
            DrawParameterItems( hasAdvancedFilter, "Control Parameters", sortedControlParameters, pNodeTypeToCreate, pParameterToReference );
            DrawParameterItems( hasAdvancedFilter, "Virtual Parameters", sortedVirtualParameters, pNodeTypeToCreate, pVirtualParameterToReference );
        }

        // Draw the node categories
        //-------------------------------------------------------------------------

        if ( pNodeTypeToCreate == nullptr )
        {
            pNodeTypeToCreate = DrawNodeTypeCategoryContextMenu( this, mouseCanvasPos, filterTokens, pOriginPin, pToolsGraphContext->GetCategorizedNodeTypes() );
        }

        // Create a selected node
        //-------------------------------------------------------------------------

        if ( pNodeTypeToCreate != nullptr )
        {
            VisualGraph::ScopedGraphModification sgm( this );

            // Create target node
            GraphNodes::FlowToolsNode* pTargetNode = nullptr;
            if ( pParameterToReference )
            {
                pTargetNode = CreateParameterReference( this, pParameterToReference );
            }
            else if ( pVirtualParameterToReference )
            {
                pTargetNode = CreateParameterReference( this, pVirtualParameterToReference );
            }
            else
            {
                pTargetNode = CreateNode( pNodeTypeToCreate );
            }

            // Set node position
            pTargetNode->SetCanvasPosition( mouseCanvasPos );

            // Try to auto-connect nodes
            if ( pSourceNode != nullptr )
            {
                VisualGraph::Flow::Pin* pTargetPin = nullptr;

                if ( pOriginPin->IsInputPin() )
                {
                    for ( auto const& outputPin : pTargetNode->GetOutputPins() )
                    {
                        if ( outputPin.m_type == pOriginPin->m_type )
                        {
                            pTargetPin = const_cast<VisualGraph::Flow::Pin*>( &outputPin );
                            break;
                        }
                    }

                    if ( pTargetPin != nullptr )
                    {
                        TryMakeConnection( pTargetNode, pTargetPin, pSourceNode, pOriginPin );
                    }
                }
                else // Output pin
                {
                    for ( auto const& inputPin : pTargetNode->GetInputPins() )
                    {
                        if ( inputPin.m_type == pOriginPin->m_type )
                        {
                            pTargetPin = const_cast<VisualGraph::Flow::Pin*>( &inputPin );
                            break;
                        }
                    }

                    if ( pTargetPin != nullptr )
                    {
                        TryMakeConnection( pSourceNode, pOriginPin, pTargetNode, pTargetPin );
                    }
                }
            }

            return true;
        }

        return false;
    }

    void FlowGraph::HandleDragAndDrop( VisualGraph::UserContext* pUserContext, ImVec2 const& mouseCanvasPos )
    {
        auto pToolsGraphContext = static_cast<ToolsGraphUserContext*>( pUserContext );

        // Handle dropped resources
        //-------------------------------------------------------------------------

        if ( ImGuiPayload const* payload = ImGui::AcceptDragDropPayload( "ResourceFile", ImGuiDragDropFlags_AcceptBeforeDelivery ) )
        {
            InlineString payloadStr = (char*) payload->Data;

            if ( !payload->IsDelivery() )
            {
                return;
            }

            //-------------------------------------------------------------------------

            ResourceID const resourceID( payloadStr.c_str() );
            if ( !resourceID.IsValid() )
            {
                return;
            }

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
                return;
            }

            //-------------------------------------------------------------------------

            VisualGraph::ScopedGraphModification sgm( this );
            auto pDataSlotNode = Cast<GraphNodes::DataSlotToolsNode>( CreateNode( pFoundNodeTypeInfo ) );
            pDataSlotNode->SetName( resourceID.GetFileNameWithoutExtension() );
            pDataSlotNode->SetDefaultResourceID( resourceID );
            pDataSlotNode->SetCanvasPosition( mouseCanvasPos );
        }

        // Handle dropped parameters
        //-------------------------------------------------------------------------

        if ( ImGuiPayload const* payload = ImGui::AcceptDragDropPayload( "AnimGraphParameter", ImGuiDragDropFlags_AcceptBeforeDelivery ) )
        {
            if ( !payload->IsDelivery() )
            {
                return;
            }

            char* pPayloadStr = (char*) payload->Data;

            for ( auto pControlParameter : pToolsGraphContext->GetControlParameters() )
            {
                if ( pControlParameter->GetParameterName().comparei( pPayloadStr ) == 0 )
                {
                    VisualGraph::ScopedGraphModification sgm( this );
                    auto pNode = CreateParameterReference( this, pControlParameter );
                    pNode->SetCanvasPosition( mouseCanvasPos );
                    return;
                }
            }

            for ( auto pVirtualParameter : pToolsGraphContext->GetVirtualParameters() )
            {
                if ( pVirtualParameter->GetParameterName().comparei( pPayloadStr ) == 0 )
                {
                    VisualGraph::ScopedGraphModification sgm( this );
                    auto pNode = CreateParameterReference( this, pVirtualParameter );
                    pNode->SetCanvasPosition( mouseCanvasPos );
                    return;
                }
            }
        }
    }
}