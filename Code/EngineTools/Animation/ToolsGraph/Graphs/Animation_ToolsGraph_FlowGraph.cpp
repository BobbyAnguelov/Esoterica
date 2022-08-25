#include "Animation_ToolsGraph_FlowGraph.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_Parameters.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_DataSlot.h"
#include "EngineTools/Core/Helpers/CategoryTree.h"
#include "EASTL/sort.h"
#include "System/TypeSystem/TypeRegistry.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    static bool DrawNodeTypeCategoryContextMenu( FlowGraph* pFlowGraph, ImVec2 const& mouseCanvasPos, Category<TypeSystem::TypeInfo const*> const& category )
    {
        EE_ASSERT( pFlowGraph != nullptr );

        auto NodeFilter = [pFlowGraph] ( CategoryItem<TypeSystem::TypeInfo const*> const& item )
        {
            // Parameter references are already handled
            if ( item.m_data->m_ID == GraphNodes::ParameterReferenceToolsNode::GetStaticTypeID() )
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

            return true;
        };

        if ( !category.HasItemsThatMatchFilter( NodeFilter ) )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        auto DrawItems = [pFlowGraph, &NodeFilter, &category, &mouseCanvasPos] ()
        {
            for ( auto const& childCategory : category.m_childCategories )
            {
                DrawNodeTypeCategoryContextMenu( pFlowGraph, mouseCanvasPos, childCategory );
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
                    VisualGraph::ScopedGraphModification sgm( pFlowGraph );
                    auto pEditorGraph = static_cast<FlowGraph*>( pFlowGraph );
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

    void FlowGraph::PostPasteNodes( TInlineVector<VisualGraph::BaseNode*, 20> const& pastedNodes )
    {
        VisualGraph::FlowGraph::PostPasteNodes( pastedNodes );
        GraphNodes::ParameterReferenceToolsNode::GloballyRefreshParameterReferences( GetRootGraph() );
    }

    void FlowGraph::DrawContextMenuOptions( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos )
    {
        auto pToolsGraphContext = static_cast<ToolsGraphUserContext*>( pUserContext );

        if ( ImGui::BeginMenu( "Control Parameters" ) )
        {
            TInlineVector<GraphNodes::ControlParameterToolsNode*, 20> sortedControlParameters = pToolsGraphContext->GetControlParameters();
            eastl::sort( sortedControlParameters.begin(), sortedControlParameters.end(), [] ( GraphNodes::ControlParameterToolsNode* const& pA, GraphNodes::ControlParameterToolsNode* const& pB ) { return strcmp( pA->GetName(), pB->GetName() ) < 0; } );

            TInlineVector<GraphNodes::VirtualParameterToolsNode*, 20> sortedVirtualParameters = pToolsGraphContext->GetVirtualParameters();
            eastl::sort( sortedVirtualParameters.begin(), sortedVirtualParameters.end(), [] ( GraphNodes::VirtualParameterToolsNode* const& pA, GraphNodes::VirtualParameterToolsNode* const& pB ) { return strcmp( pA->GetName(), pB->GetName() ) < 0; } );

            //-------------------------------------------------------------------------

            if ( pToolsGraphContext->GetControlParameters().empty() && pToolsGraphContext->GetVirtualParameters().empty() )
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
                        VisualGraph::ScopedGraphModification sgm( this );
                        auto pNode = CreateNode<GraphNodes::ParameterReferenceToolsNode>( pParameter );
                        pNode->SetCanvasPosition( mouseCanvasPos );
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
                        VisualGraph::ScopedGraphModification sgm( this );
                        auto pNode = CreateNode<GraphNodes::ParameterReferenceToolsNode>( pParameter );
                        pNode->SetCanvasPosition( mouseCanvasPos );
                    }
                    ImGui::PopStyleColor();
                }
            }

            ImGui::EndMenu();
        }

        //-------------------------------------------------------------------------

        DrawNodeTypeCategoryContextMenu( this, mouseCanvasPos, pToolsGraphContext->GetCategorizedNodeTypes() );
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

            InlineString payloadStr = (char*) payload->Data;

            for ( auto pControlParameter : pToolsGraphContext->GetControlParameters() )
            {
                if ( payloadStr == pControlParameter->GetName() )
                {
                    VisualGraph::ScopedGraphModification sgm( this );
                    auto pNode = CreateNode<GraphNodes::ParameterReferenceToolsNode>( pControlParameter );
                    pNode->SetCanvasPosition( mouseCanvasPos );
                    return;
                }
            }

            for ( auto pVirtualParameter : pToolsGraphContext->GetVirtualParameters() )
            {
                if ( payloadStr == pVirtualParameter->GetName() )
                {
                    VisualGraph::ScopedGraphModification sgm( this );
                    auto pNode = CreateNode<GraphNodes::ParameterReferenceToolsNode>( pVirtualParameter );
                    pNode->SetCanvasPosition( mouseCanvasPos );
                    return;
                }
            }
        }
    }
}