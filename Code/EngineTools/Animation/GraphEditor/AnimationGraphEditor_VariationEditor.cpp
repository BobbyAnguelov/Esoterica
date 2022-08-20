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

    //-------------------------------------------------------------------------

    void GraphVariationEditor::StartCreate( StringID variationID )
    {
        EE_ASSERT( variationID.IsValid() );
        m_activeOperationVariationID = variationID;
        m_activeOperation = OperationType::Create;
        strncpy_s( m_buffer, "NewChildVariation", 255 );
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

    //-------------------------------------------------------------------------

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
                DrawDialogs();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void GraphVariationEditor::DrawVariationTreeNode( VariationHierarchy const& variationHierarchy, StringID variationID )
    {
        ImGui::PushID( variationID.GetID() );

        auto const childVariations = variationHierarchy.GetChildVariations( variationID );
        bool const isSelected = m_editorContext.GetSelectedVariationID() == variationID;

        // Draw Tree Node
        //-------------------------------------------------------------------------

        bool isTreeNodeOpen = false;
        {
            ImGuiX::ScopedFont const sf( isSelected ? ImGuiX::Font::SmallBold : ImGuiX::Font::Small, isSelected ? ImGuiX::ConvertColor( Colors::LimeGreen ) : ImGui::GetStyle().Colors[ImGuiCol_Text] );

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
            if ( childVariations.empty() )
            {
                flags |= ( ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet );
            }

            ImGui::SetNextItemOpen( true );
            isTreeNodeOpen = ImGui::TreeNodeEx( variationID.c_str(), flags );
            if ( ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
            {
                m_editorContext.SetSelectedVariation( variationID );
            }
        }

        // Context Menu
        //-------------------------------------------------------------------------

        if ( ImGui::BeginPopupContextItem( variationID.c_str() ) )
        {
            if ( ImGui::MenuItem( "Set Active" ) )
            {
                m_editorContext.SetSelectedVariation( variationID );
            }

            //-------------------------------------------------------------------------

            if ( ImGui::MenuItem( "Create Child Variation" ) )
            {
                StartCreate( variationID );
            }

            if ( variationID != Variation::s_defaultVariationID )
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
        DrawVariationTreeNode( m_editorContext.GetVariationHierarchy(), Variation::s_defaultVariationID );
    }

    void GraphVariationEditor::DrawOverridesTable()
    {
        auto pRootGraph = m_editorContext.GetRootGraph();
        auto dataSlotNodes = pRootGraph->FindAllNodesOfType<GraphNodes::DataSlotEditorNode>( VisualGraph::SearchMode::Recursive, VisualGraph::SearchTypeMatch::Derived );
        bool isDefaultVariationSelected = m_editorContext.IsDefaultVariationSelected();

        //-------------------------------------------------------------------------

        ImGui::AlignTextToFramePadding();
        ImGui::Text( "Skeleton:" );
        ImGui::SameLine( 0, 0 );

        auto pVariation = m_editorContext.GetVariation( m_editorContext.GetSelectedVariationID() );
        ResourceID resourceID = pVariation->m_pSkeleton.GetResourceID();
        if ( m_resourcePicker.DrawResourcePicker( Skeleton::GetStaticResourceTypeID(), &resourceID, true ) )
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
            ImGui::TableSetupColumn( "##Override", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 26 );
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
                    if ( m_resourcePicker.DrawResourcePicker( pDataSlotNode->GetSlotResourceTypeID(), pResourceID, true ) )
                    {
                        VisualGraph::ScopedGraphModification sgm( pRootGraph );
                        pDataSlotNode->SetOverrideValueForVariation( currentVariationID, m_resourcePicker.GetSelectedResourceID() );
                    }
                }
                else // Variation
                {
                    // If we have an override for this variation
                    if ( pDataSlotNode->HasOverrideForVariation( currentVariationID ) )
                    {
                        ResourceID* pResourceID = pDataSlotNode->GetOverrideValueForVariation( currentVariationID );
                        if ( m_resourcePicker.DrawResourcePicker( AnimationClip::GetStaticResourceTypeID(), pResourceID, true ) )
                        {
                            VisualGraph::ScopedGraphModification sgm( pRootGraph );
                            pDataSlotNode->SetOverrideValueForVariation( currentVariationID, m_resourcePicker.GetSelectedResourceID() );
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
                        if ( ImGuiX::ColoredButton( ImGuiX::ConvertColor( Colors::MediumRed ), ImGuiX::ConvertColor( Colors::White ), EE_ICON_CLOSE_CIRCLE, ImVec2( 26, 24 ) ) )
                        {
                            pDataSlotNode->RemoveOverride( currentVariationID );
                        }
                    }
                    else // Create an override
                    {
                        if ( ImGuiX::ColoredButton( ImGuiX::ConvertColor( Colors::ForestGreen ), ImGuiX::ConvertColor( Colors::White ), EE_ICON_PLUS, ImVec2( 26, 24 ) ) )
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

    void GraphVariationEditor::DrawDialogs()
    {
        auto pRootGraph = m_editorContext.GetRootGraph();
        bool isDialogOpen = m_activeOperation != OperationType::None;

        // Creation
        //-------------------------------------------------------------------------

        if ( m_activeOperation == OperationType::Create )
        {
            ImGui::OpenPopup( "Create" );
            if ( ImGui::BeginPopupModal( "Create", &isDialogOpen, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize ) )
            {
                if ( DrawVariationNameEditUI() )
                {
                    // Only allow creations of unique variation names
                    StringID newVariationID( m_buffer );
                    EE_ASSERT( !m_editorContext.IsValidVariation( newVariationID ) );

                    // Create new variation
                    m_editorContext.CreateVariation( newVariationID, m_activeOperationVariationID );
                    m_activeOperationVariationID = StringID();
                    m_activeOperation = OperationType::None;

                    // Switch to the new variation
                    m_editorContext.SetSelectedVariation( newVariationID );
                }

                ImGui::EndPopup();
            }
        }

        // Rename
        //-------------------------------------------------------------------------

        if ( m_activeOperation == OperationType::Rename )
        {
            ImGui::OpenPopup( "Rename" );
            if ( ImGui::BeginPopupModal( "Rename", &isDialogOpen, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize ) )
            {
                if ( DrawVariationNameEditUI() )
                {
                    bool const isRenamingCurrentlySelectedVariation = ( m_activeOperationVariationID == m_editorContext.GetSelectedVariationID() );

                    // Only allow rename to unique variation names
                    StringID newVariationID( m_buffer );
                    EE_ASSERT( !m_editorContext.IsValidVariation( newVariationID ) );

                    // Perform rename
                    m_editorContext.RenameVariation( m_activeOperationVariationID, newVariationID );
                    m_activeOperationVariationID = StringID();
                    m_activeOperation = OperationType::None;

                    // Set the selected variation to the renamed variation
                    if ( isRenamingCurrentlySelectedVariation )
                    {
                        m_editorContext.SetSelectedVariation( newVariationID );
                    }
                }

                ImGui::EndPopup();
            }
        }

        // Deletion
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
                    EE_ASSERT( m_activeOperationVariationID != Variation::s_defaultVariationID );

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

    //-------------------------------------------------------------------------

    static bool IsValidVariationNameChar( ImWchar c )
    {
        return isalnum( c ) || c == '_';
    }

    static int FilterVariationNameChars( ImGuiInputTextCallbackData* data )
    {
        if ( IsValidVariationNameChar( data->EventChar ) )
        {
            return 0;
        }
        return 1;
    }

    bool GraphVariationEditor::DrawVariationNameEditUI()
    {
        bool const isRenameOp = ( m_activeOperation == OperationType::Rename );
        bool nameChangeConfirmed = false;

        // Validate current buffer
        //-------------------------------------------------------------------------

        auto ValidateVariationName = [this, isRenameOp] ()
        {
            size_t const bufferLen = strlen( m_buffer );

            if ( bufferLen == 0 )
            {
                return false;
            }

            // Check for invalid chars
            for ( auto i  = 0; i < bufferLen; i++ )
            {
                if ( !IsValidVariationNameChar( m_buffer[i] ) )
                {
                    return false;
                }
            }

            // Check for existing variations with the same name but different casing
            String newVariationName( m_buffer );
            for ( auto const& variation : m_editorContext.GetVariationHierarchy().GetAllVariations() )
            {
                int32_t const result = newVariationName.comparei( variation.m_ID.c_str() );
                if ( result == 0 )
                {
                    return false;
                }
            }

            return true;
        };

        // Input Field
        //-------------------------------------------------------------------------

        bool isValidVariationName = ValidateVariationName();
        ImGui::PushStyleColor( ImGuiCol_Text, isValidVariationName ? ImGui::GetStyle().Colors[ImGuiCol_Text] : ImGuiX::ConvertColor(Colors::Red).Value );
        if ( ImGui::InputText( "##VariationName", m_buffer, 255, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CallbackCharFilter, FilterVariationNameChars ) )
        {
            nameChangeConfirmed = true;
        }
        ImGui::PopStyleColor();
        ImGui::NewLine();

        float const dialogWidth = ( ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin() ).x;
        ImGui::SameLine( 0, dialogWidth - 104 );

        // Buttons
        //-------------------------------------------------------------------------

        ImGui::BeginDisabled( !isValidVariationName );
        if ( ImGui::Button( "Ok", ImVec2( 50, 0 ) ) )
        {
            nameChangeConfirmed = true;
        }
        ImGui::EndDisabled();

        ImGui::SameLine( 0, 4 );

        if ( ImGui::Button( "Cancel", ImVec2( 50, 0 ) ) )
        {
            m_activeOperation = OperationType::None;
            nameChangeConfirmed = false;
        }

        // Final validation
        //-------------------------------------------------------------------------

        if ( nameChangeConfirmed )
        {
            if ( !ValidateVariationName() )
            {
                nameChangeConfirmed = false;
            }
        }

        return nameChangeConfirmed;
    }
}