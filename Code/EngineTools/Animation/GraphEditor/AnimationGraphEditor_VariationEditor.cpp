#include "AnimationGraphEditor_VariationEditor.h"
#include "AnimationGraphEditor_Context.h"
#include "EditorGraph/Animation_EditorGraph_Variations.h"
#include "EditorGraph/Animation_EditorGraph_Definition.h"
#include "EngineTools/Resource/ResourceFilePicker.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"
#include "System/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    GraphVariationEditor::GraphVariationEditor( GraphEditorContext& editorContext )
        : m_editorContext( editorContext )
        , m_resourcePicker( *editorContext.GetToolsContext() )
    {}

    void GraphVariationEditor::UpdateAndDraw( UpdateContext const& context, ImGuiWindowClass* pWindowClass, char const* pWindowName )
    {
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4, 4 ) );
        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( pWindowName, nullptr, 0 ) )
        {
            if ( ImGui::BeginTable( "VariationMainSplitter", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX ) )
            {
                ImGui::TableSetupColumn( "VariationTree", ImGuiTableColumnFlags_WidthStretch, 0.2f );
                ImGui::TableSetupColumn( "Data", ImGuiTableColumnFlags_WidthStretch );

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                DrawVariationTree();

                ImGui::TableNextColumn();
                DrawOverridesTable();

                ImGui::EndTable();
            }

            //-------------------------------------------------------------------------

            if ( m_activeOperation != OperationType::None )
            {
                DrawActiveOperationUI();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void GraphVariationEditor::DrawVariationTreeNode( VariationHierarchy const& variationHierarchy, StringID variationID )
    {
        ImGui::PushID( variationID.GetID() );

        // Open Tree Node
        //-------------------------------------------------------------------------

        bool const isSelected = m_editorContext.GetSelectedVariationID() == variationID;

        ImGuiX::PushFontAndColor( isSelected ? ImGuiX::Font::SmallBold : ImGuiX::Font::Small, isSelected ? ImGuiX::ConvertColor( Colors::LimeGreen ) : ImGui::GetStyle().Colors[ImGuiCol_Text] );
        bool const isTreeNodeOpen = ImGui::TreeNode( variationID.c_str() );
        ImGui::PopFont();
        ImGui::PopStyleColor();

        // Click Handler
        //-------------------------------------------------------------------------

        if ( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
        {
            m_editorContext.SetSelectedVariation( variationID );
        }

        // Context Menu
        //-------------------------------------------------------------------------

        if ( ImGui::BeginPopupContextItem( variationID.c_str() ) )
        {
            if ( ImGui::MenuItem( "Create Child" ) )
            {
                StartCreate( variationID );
            }

            if ( variationID != GraphVariation::DefaultVariationID )
            {
                ImGui::Separator();

                if ( ImGui::MenuItem( "Rename" ) )
                {
                    StartRename( variationID );
                }

                if ( ImGui::MenuItem( "Delete" ) )
                {
                    StartDelete( variationID );
                }
            }

            ImGui::EndPopup();
        }

        // Draw node contents
        //-------------------------------------------------------------------------

        if( isTreeNodeOpen )
        {
            auto const childVariations = variationHierarchy.GetChildVariations( variationID );
            for ( StringID const& childVariationID : childVariations )
            {
                DrawVariationTreeNode( variationHierarchy, childVariationID );
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    void GraphVariationEditor::DrawVariationTree()
    {
        DrawVariationTreeNode( m_editorContext.GetVariationHierarchy(), GraphVariation::DefaultVariationID );
    }

    void GraphVariationEditor::DrawOverridesTable()
    {
        auto pRootGraph = m_editorContext.GetRootGraph();
        auto dataSlotNodes = pRootGraph->FindAllNodesOfType<GraphNodes::DataSlotEditorNode>( VisualGraph::SearchMode::Recursive, VisualGraph::SearchTypeMatch::Derived );
        bool isDefaultVariationSelected = m_editorContext.IsDefaultVariationSelected();

        //-------------------------------------------------------------------------

        auto c = ImGui::GetContentRegionAvail();
        ImGui::AlignTextToFramePadding();
        ImGui::Text( "Skeleton: " );
        ImGui::SameLine( 0, 4 );

        auto pVariation = m_editorContext.GetVariation( m_editorContext.GetSelectedVariationID() );
        ResourceID resourceID = pVariation->m_pSkeleton.GetResourceID();
        if ( m_resourcePicker.DrawResourcePicker( Skeleton::GetStaticResourceTypeID(), &resourceID ) )
        {
            VisualGraph::ScopedGraphModification sgm( pRootGraph );

            if ( m_resourcePicker.GetSelectedResourceID().IsValid() )
            {
                pVariation->m_pSkeleton = m_resourcePicker.GetSelectedResourceID();
            }
            else
            {
                pVariation->m_pSkeleton.Clear();
            }
        }

        //-------------------------------------------------------------------------

        if ( ImGui::BeginTable( "SourceTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX ) )
        {
            ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Path", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Source", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "##Override", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 20 );
            ImGui::TableHeadersRow();

            //-------------------------------------------------------------------------

            StringID const currentVariationID = m_editorContext.GetSelectedVariationID();

            for ( auto pDataSlotNode : dataSlotNodes )
            {
                bool const hasOverrideForVariation = pDataSlotNode->HasOverrideForVariation( currentVariationID );

                ImGui::PushID( pDataSlotNode );

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                if ( !isDefaultVariationSelected && hasOverrideForVariation )
                {
                    ImGui::TextColored( ImVec4( 0, 1, 0, 1 ), pDataSlotNode->GetName() );
                }
                else
                {
                    ImGui::Text( pDataSlotNode->GetName() );
                }

                ImGui::TableNextColumn();
                ImGui::Text( pDataSlotNode->GetPathFromRoot().c_str() );

                ImGui::TableNextColumn();

                // Default variations always have values created
                if ( isDefaultVariationSelected )
                {
                    ResourceID* pResourceID = pDataSlotNode->GetOverrideValueForVariation( currentVariationID );
                    if ( m_resourcePicker.DrawResourcePicker( pDataSlotNode->GetSlotResourceTypeID(), pResourceID) )
                    {
                        VisualGraph::ScopedGraphModification sgm( pRootGraph );
                        *pResourceID = m_resourcePicker.GetSelectedResourceID();
                    }
                }
                else // Variation
                {
                    // If we have an override for this variation
                    if ( pDataSlotNode->HasOverrideForVariation( currentVariationID ) )
                    {
                        ResourceID* pResourceID = pDataSlotNode->GetOverrideValueForVariation( currentVariationID );
                        if ( m_resourcePicker.DrawResourcePicker( AnimationClip::GetStaticResourceTypeID(), pResourceID ) )
                        {
                            VisualGraph::ScopedGraphModification sgm( pRootGraph );

                            // If we've cleared the resource ID and it's not the default, remove the override
                            if ( !pResourceID->IsValid() && !m_editorContext.IsDefaultVariationSelected() )
                            {
                                pDataSlotNode->RemoveOverride( currentVariationID );
                            }
                            else
                            {
                                *pResourceID = m_resourcePicker.GetSelectedResourceID();
                            }
                        }
                    }
                    else // Show current value
                    {
                        ImGui::Text( pDataSlotNode->GetResourceID( m_editorContext.GetVariationHierarchy(), currentVariationID ).c_str() );
                    }
                }

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                if ( !isDefaultVariationSelected )
                {
                    if ( pDataSlotNode->HasOverrideForVariation( currentVariationID ) )
                    {
                        if ( ImGuiX::ColoredButton( ImGuiX::ConvertColor( Colors::MediumRed ), ImGuiX::ConvertColor( Colors::White ), EE_ICON_CLOSE_CIRCLE, ImVec2( 22, 22 ) ) )
                        {
                            pDataSlotNode->RemoveOverride( currentVariationID );
                        }
                    }
                    else // Create an override
                    {
                        if ( ImGuiX::ColoredButton( ImGuiX::ConvertColor( Colors::ForestGreen ), ImGuiX::ConvertColor( Colors::White ), EE_ICON_PLUS, ImVec2( 22, 22 ) ) )
                        {
                            pDataSlotNode->CreateOverride( currentVariationID );
                        }
                    }
                }

                ImGui::PopID();
            }

            //-------------------------------------------------------------------------

            ImGui::EndTable();
        }
    }

    //-------------------------------------------------------------------------

    void GraphVariationEditor::StartCreate( StringID variationID )
    {
        EE_ASSERT( variationID.IsValid() );
        m_activeOperationVariationID = variationID;
        m_activeOperation = OperationType::Create;
        strncpy_s( m_buffer, "New Child Variation", 255 );
    }

    void GraphVariationEditor::StartRename( StringID variationID )
    {
        EE_ASSERT( variationID.IsValid() );
        m_activeOperationVariationID = variationID;
        m_activeOperation = OperationType::Rename;
        strncpy_s( m_buffer, variationID.c_str(), 255 );
    }

    void GraphVariationEditor::StartDelete( StringID variationID )
    {
        EE_ASSERT( variationID.IsValid() );
        m_activeOperationVariationID = variationID;
        m_activeOperation = OperationType::Delete;
    }

    void GraphVariationEditor::DrawActiveOperationUI()
    {
        auto pRootGraph = m_editorContext.GetRootGraph();

        bool isDialogOpen = m_activeOperation != OperationType::None;

        if ( m_activeOperation == OperationType::Create )
        {
            ImGui::OpenPopup( "Create" );
            if ( ImGui::BeginPopupModal( "Create", &isDialogOpen, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize ) )
            {
                bool nameChangeConfirmed = false;

                ImGui::PushStyleColor( ImGuiCol_Text, m_editorContext.IsValidVariation( StringID( m_buffer ) ) ? ImGuiX::ConvertColor( Colors::Red ).Value : ImGui::GetStyle().Colors[ImGuiCol_Text] );
                if ( ImGui::InputText( "##VariationName", m_buffer, 255, ImGuiInputTextFlags_EnterReturnsTrue ) )
                {
                    nameChangeConfirmed = true;
                }
                ImGui::PopStyleColor();
                ImGui::NewLine();

                float const dialogWidth = ( ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin() ).x;
                ImGui::SameLine( 0, dialogWidth - 104 );

                if ( ImGui::Button( "Ok", ImVec2( 50, 0 ) ) || nameChangeConfirmed )
                {
                    // Only allow creations of unique variation names
                    StringID newVariationID( m_buffer );
                    if ( !m_editorContext.IsValidVariation( newVariationID ) )
                    {
                        m_editorContext.CreateVariation( newVariationID, m_activeOperationVariationID );
                        m_activeOperationVariationID = StringID();
                        m_activeOperation = OperationType::None;
                    }
                }

                ImGui::SameLine( 0, 4 );

                if ( ImGui::Button( "Cancel", ImVec2( 50, 0 ) ) )
                {
                    m_activeOperation = OperationType::None;
                }

                ImGui::EndPopup();
            }
        }

        //-------------------------------------------------------------------------

        if ( m_activeOperation == OperationType::Rename )
        {
            ImGui::OpenPopup( "Rename" );
            if ( ImGui::BeginPopupModal( "Rename", &isDialogOpen, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize ) )
            {
                bool nameChangeConfirmed = false;

                ImGui::PushStyleColor( ImGuiCol_Text, m_editorContext.IsValidVariation( StringID( m_buffer ) ) ? ImGuiX::ConvertColor( Colors::Red ).Value : ImGui::GetStyle().Colors[ImGuiCol_Text] );
                if ( ImGui::InputText( "##VariationName", m_buffer, 255, ImGuiInputTextFlags_EnterReturnsTrue ) )
                {
                    nameChangeConfirmed = true;
                }
                ImGui::PopStyleColor();
                ImGui::NewLine();

                float const dialogWidth = ( ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin() ).x;
                ImGui::SameLine( 0, dialogWidth - 104 );

                if ( ImGui::Button( "Ok", ImVec2( 50, 0 ) ) || nameChangeConfirmed )
                {
                    // Only allow rename to unique variation names
                    StringID newVariationID( m_buffer );
                    if ( !m_editorContext.IsValidVariation( newVariationID ) )
                    {
                        bool const isRenamingSelectedVariation = m_activeOperationVariationID == m_editorContext.GetSelectedVariationID();

                        m_editorContext.RenameVariation( m_activeOperationVariationID, newVariationID );
                        m_activeOperationVariationID = StringID();
                        m_activeOperation = OperationType::None;

                        // Set the selected variation to the renamed variation
                        if ( isRenamingSelectedVariation )
                        {
                            m_editorContext.SetSelectedVariation( newVariationID );
                        }
                    }
                }

                ImGui::SameLine( 0, 4 );

                if ( ImGui::Button( "Cancel", ImVec2( 50, 0 ) ) )
                {
                    m_activeOperation = OperationType::None;
                }

                ImGui::EndPopup();
            }
        }

        //-------------------------------------------------------------------------

        if ( m_activeOperation == OperationType::Delete )
        {
            ImGui::OpenPopup( "Delete" );
            if ( ImGui::BeginPopupModal( "Delete", &isDialogOpen, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize ) )
            {
                ImGui::Text( "Are you sure you want to delete this variation?" );
                ImGui::NewLine();

                float const dialogWidth = ( ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin() ).x;
                ImGui::SameLine( 0, dialogWidth - 64 );

                if ( ImGui::Button( "Yes", ImVec2( 30, 0 ) ) )
                {
                    EE_ASSERT( m_activeOperationVariationID != GraphVariation::DefaultVariationID );

                    // Update selection
                    auto const pVariation = m_editorContext.GetVariation( m_activeOperationVariationID );
                    m_editorContext.SetSelectedVariation( pVariation->m_parentID );

                    // Destroy variation
                    m_editorContext.DestroyVariation( m_activeOperationVariationID );
                    m_activeOperationVariationID = StringID();
                    m_activeOperation = OperationType::None;
                }

                ImGui::SameLine( 0, 4 );

                if ( ImGui::Button( "No", ImVec2( 30, 0 ) ) )
                {
                    m_activeOperation = OperationType::None;
                }

                ImGui::EndPopup();
            }
        }

        //-------------------------------------------------------------------------

        if ( !isDialogOpen )
        {
            m_activeOperation = OperationType::None;
        }
    }
}