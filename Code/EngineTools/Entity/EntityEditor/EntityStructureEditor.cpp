#include "EntityStructureEditor.h"
#include "EntityUndoableAction.h"
#include "EngineTools/Core/ToolsContext.h"
#include "Engine/UpdateContext.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntitySystem.h"
#include "System/TypeSystem/TypeRegistry.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    using namespace TypeSystem;

    //-------------------------------------------------------------------------

    class SpatialComponentItem final : public TreeListViewItem
    {
    public:

        constexpr static char const* const s_dragAndDropID = "ComponentItem";

    public:

        SpatialComponentItem( SpatialEntityComponent* pComponent )
            : TreeListViewItem()
            , m_pComponent( pComponent )
            , m_componentID( pComponent->GetID() )
        {
            EE_ASSERT( m_pComponent != nullptr );
            m_tooltip.sprintf( "Type: %s", m_pComponent->GetTypeInfo()->GetFriendlyTypeName() );
        }

        virtual StringID GetNameID() const override { return m_pComponent->GetNameID(); }
        virtual uint64_t GetUniqueID() const override { return m_componentID.m_value; }
        virtual bool HasContextMenu() const override { return true; }
        virtual char const* GetTooltipText() const override { return m_tooltip.c_str(); }
        virtual bool IsDragAndDropSource() const override { return !m_pComponent->IsRootComponent(); }
        virtual bool IsDragAndDropTarget() const override { return true; }

        virtual void SetDragAndDropPayloadData() const override
        {
            uintptr_t itemAddress = uintptr_t( this );
            ImGui::SetDragDropPayload( s_dragAndDropID, &itemAddress, sizeof( uintptr_t ) );
        }

    public:

        SpatialEntityComponent*     m_pComponent = nullptr;
        ComponentID                 m_componentID; // Cached since we want to access this on rebuild without touching the component memory which might have been invalidated
        String                      m_tooltip;
    };

    //-------------------------------------------------------------------------

    EntityStructureEditor::EntityStructureEditor( ToolsContext const* pToolsContext, UndoStack* pUndoStack, EntityWorld* pWorld )
        : TreeListView()
        , m_pToolsContext( pToolsContext )
        , m_pUndoStack( pUndoStack )
        , m_pWorld( pWorld )
        , m_allSystemTypes( pToolsContext->m_pTypeRegistry->GetAllDerivedTypes( EntitySystem::GetStaticTypeID(), false, false, true ) )
        , m_allComponentTypes( pToolsContext->m_pTypeRegistry->GetAllDerivedTypes( EntityComponent::GetStaticTypeID(), false, false, true ) )
    {
        EE_ASSERT( m_pUndoStack != nullptr );
        EE_ASSERT( m_pWorld != nullptr );

        m_expandItemsOnlyViaArrow = true;
        m_multiSelectionAllowed = true;
        m_drawBorders = false;
        m_drawRowBackground = false;
        m_useSmallFont = false;
        m_showBulletsOnLeaves = true;

        // Dialogs
        //-------------------------------------------------------------------------

        Memory::MemsetZero( m_operationBuffer, 256 * sizeof( char ) );

        // Split components
        //-------------------------------------------------------------------------

        for ( int32_t i = 0; i < m_allComponentTypes.size(); i++ )
        {
            if ( m_allComponentTypes[i]->IsDerivedFrom( SpatialEntityComponent::GetStaticTypeID() ) )
            {
                m_allSpatialComponentTypes.emplace_back( m_allComponentTypes[i] );
                m_allComponentTypes.erase( m_allComponentTypes.begin() + i );
                i--;
            }
        }
    }

    EntityStructureEditor::~EntityStructureEditor()
    {
        EE_ASSERT( m_pActiveUndoAction == nullptr );
    }

    void EntityStructureEditor::Initialize( UpdateContext const& context, uint32_t widgetUniqueID )
    {
        m_windowName.sprintf( "Entity##%u", widgetUniqueID );
        m_entityStateChangedBindingID = Entity::OnEntityUpdated().Bind( [this] ( Entity* pEntityChanged ) { EntityUpdateDetected( pEntityChanged ); } );
    }

    void EntityStructureEditor::Shutdown( UpdateContext const& context )
    {
        Entity::OnEntityUpdated().Unbind( m_entityStateChangedBindingID );

        if ( m_pActiveUndoAction != nullptr )
        {
            EE::Delete( m_pActiveUndoAction );
        }
    }

    //-------------------------------------------------------------------------
    // General
    //-------------------------------------------------------------------------

    void EntityStructureEditor::ClearEditedEntityState()
    {
        m_editedEntities.clear();
        m_editedEntityID.Clear();
        m_pEditedEntity = nullptr;
        m_initiallySelectedComponentNameID.Clear();
        m_selectedComponents.clear();
        m_selectedSpatialComponents.clear();
        DestroyTree();
    }

    void EntityStructureEditor::SetEntityToEdit( Entity* pEntity, EntityComponent* pInitiallySelectedComponent )
    {
        ClearEditedEntityState();

        if ( pEntity != nullptr )
        {
            m_editedEntities.emplace_back( pEntity->GetID() );
            m_editedEntityID = pEntity->GetID();
        }

        //-------------------------------------------------------------------------

        if ( pInitiallySelectedComponent != nullptr )
        {
            EE_ASSERT( pEntity != nullptr );
            EE_ASSERT( pInitiallySelectedComponent->GetEntityID() == pEntity->GetID() );
            m_initiallySelectedComponentNameID = pInitiallySelectedComponent->GetNameID();
        }

        //-------------------------------------------------------------------------

        RebuildTree( false ); // Destroy current tree
        m_shouldRefreshEditorState = true;
    }

    void EntityStructureEditor::SetEntitiesToEdit( TVector<Entity*> const& entities )
    {
        if ( entities.size() == 1 )
        {
            return SetEntityToEdit( entities[0] );
        }

        //-------------------------------------------------------------------------

        ClearEditedEntityState();

        for ( auto pEntity : entities )
        {
            m_editedEntities.emplace_back( pEntity->GetID() );
        }

        //-------------------------------------------------------------------------

        RebuildTree( false ); // Destroy current tree
        m_shouldRefreshEditorState = true;
    }

    void EntityStructureEditor::EntityUpdateDetected( Entity* pEntityChanged )
    {
        m_shouldRefreshEditorState = ( pEntityChanged == m_pEditedEntity );
    }

    bool EntityStructureEditor::UpdateAndDraw( UpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        // Handle active operations
        //-------------------------------------------------------------------------

        if ( m_pActiveUndoAction != nullptr )
        {
            EE_ASSERT( m_pActiveUndoActionEntity != nullptr );

            if ( !m_pActiveUndoActionEntity->HasStateChangeActionsPending() )
            {
                m_pActiveUndoAction->RecordEndEdit();
                m_pUndoStack->RegisterAction( m_pActiveUndoAction );
                m_pActiveUndoAction = nullptr;
                m_pActiveUndoActionEntity = nullptr;
            }

            m_shouldRefreshEditorState = true;
        }

        // Refresh entity editor state
        //-------------------------------------------------------------------------

        if ( m_pWorld->HasPendingMapChangeActions() )
        {
            m_shouldRefreshEditorState = true;
            m_pEditedEntity = nullptr;
            m_selectedComponents.clear();
            m_selectedSpatialComponents.clear();
        }
        else // No pending action, safe to refresh
        {
            // Check if we should auto refresh
            if ( m_editedEntityID.IsValid() )
            {
                auto pEntity = m_pWorld->FindEntity( m_editedEntityID );
                if( pEntity == nullptr || pEntity->HasStateChangeActionsPending() )
                {
                    m_shouldRefreshEditorState = true;
                    m_pEditedEntity = nullptr;
                    m_selectedComponents.clear();
                    m_selectedSpatialComponents.clear();
                }
            }

            //-------------------------------------------------------------------------

            if ( m_shouldRefreshEditorState )
            {
                // Resolve edited entity ptr
                if ( m_editedEntityID.IsValid() )
                {
                    // Resolve entity ID only once map isnt messing with entity state
                    m_pEditedEntity = m_pWorld->FindEntity( m_editedEntityID );
                    if ( m_pEditedEntity != nullptr )
                    {
                        // Only refresh tree once all entity actions have completed
                        if ( !m_pEditedEntity->HasStateChangeActionsPending() )
                        {
                            RebuildTree();
                            m_shouldRefreshEditorState = false;

                            // Set initial selection state
                            if ( m_initiallySelectedComponentNameID.IsValid() )
                            {
                                auto pComponent = m_pEditedEntity->FindComponentByName( m_initiallySelectedComponentNameID );
                                EE_ASSERT( pComponent );
                                auto pItem = FindItem( pComponent->GetID().m_value );
                                EE_ASSERT( pItem != nullptr );
                                TreeListView::SetSelection( pItem );

                                m_initiallySelectedComponentNameID.Clear();
                            }
                        }
                    }
                    else // Entity no longer exists, so stop editing it
                    {
                        ClearEditedEntityState();
                    }
                }
                else
                {
                    m_shouldRefreshEditorState = false;
                }
            }
        }

        // Draw UI
        //-------------------------------------------------------------------------

        bool isFocused = false;
        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( m_windowName.c_str() ) )
        {
            if ( !m_shouldRefreshEditorState && m_pEditedEntity != nullptr )
            {
                //-------------------------------------------------------------------------
                // Header
                //-------------------------------------------------------------------------

                {
                    ImGuiX::ScopedFont sf( ImGuiX::Font::MediumBold );
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text( "Entity: %s", m_pEditedEntity->GetNameID().c_str() );
                }

                ImGui::SameLine( ImGui::GetContentRegionAvail().x + ImGui::GetStyle().ItemSpacing.x - 28, 0 );
                if ( ImGui::Button( EE_ICON_PLAYLIST_EDIT, ImVec2( 26, 26 ) ) )
                {
                    ClearSelection();
                    m_onRequestedTypeToEditChanged.Execute( m_pEditedEntity );
                }
                ImGuiX::ItemTooltip( "Edit Entity Details" );
                ImGui::Separator();

                //-------------------------------------------------------------------------

                ImVec2 const availableArea = ImGui::GetContentRegionAvail();
                ImVec2 const addButtonSize = ImVec2( 60, 26 );
                float const sectionHeaderHeight = addButtonSize.y + ( ImGui::GetStyle().ItemSpacing.y * 2 );
                float const listWidgetHeight = ( availableArea.y - ( sectionHeaderHeight * 3 ) ) / 3;

                //-------------------------------------------------------------------------
                // Spatial Components
                //-------------------------------------------------------------------------

                ImGui::AlignTextToFramePadding();
                ImGui::Text( EE_ICON_AXIS_ARROW" Spatial Components" );
                ImGui::SameLine( availableArea.x - 60 );
                {
                    ImGuiX::ScopedFont sf( ImGuiX::Font::SmallBold );
                    if ( ImGuiX::ColoredButton( ImGuiX::ImColors::Green, ImGuiX::ImColors::White, EE_ICON_AXIS_ARROW" ADD", addButtonSize ) )
                    {
                        m_pOperationTargetComponent = nullptr;
                        StartOperation( Operation::AddSpatialComponent );
                    }
                    ImGuiX::ItemTooltip( "Add Spatial Component" );
                }

                auto const& style = ImGui::GetStyle();
                ImGui::PushStyleColor( ImGuiCol_ChildBg, style.Colors[ImGuiCol_FrameBg] );
                ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, style.FrameRounding );
                ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, style.FrameBorderSize );
                ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, style.FramePadding );
                if ( ImGui::BeginChild( "ST", ImVec2( -1, listWidgetHeight ), true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding ) )
                {
                    TreeListView::UpdateAndDraw( -1.0f );

                    if ( GetNumItems() == 0 )
                    {
                        ImGui::Text( "No Components" );
                    }
                }
                ImGui::EndChild();
                ImGui::PopStyleVar( 3 );
                ImGui::PopStyleColor();

                //-------------------------------------------------------------------------
                // Components
                //-------------------------------------------------------------------------

                ImGui::AlignTextToFramePadding();
                ImGui::Text( EE_ICON_DATABASE" Components" );
                ImGui::SameLine( availableArea.x - 60 );
                {
                    ImGuiX::ScopedFont sf( ImGuiX::Font::SmallBold );
                    if ( ImGuiX::ColoredButton( ImGuiX::ImColors::Green, ImGuiX::ImColors::White, EE_ICON_DATABASE" ADD", addButtonSize ) )
                    {
                        m_pOperationTargetComponent = nullptr;
                        StartOperation( Operation::AddComponent );
                    }
                    ImGuiX::ItemTooltip( "Add Component" );
                }

                if( ImGui::BeginListBox( "Components", ImVec2( -1, listWidgetHeight ) ) )
                {
                    for ( auto pComponent : m_pEditedEntity->GetComponents() )
                    {
                        if ( IsOfType<SpatialEntityComponent>( pComponent ) )
                        {
                            continue;
                        }

                        ImGui::PushID( pComponent );

                        bool const wasSelected = VectorContains( m_selectedComponents, pComponent );
                        if ( wasSelected )
                        {
                            ImGui::PushStyleColor( ImGuiCol_Text, ImGuiX::Style::s_colorAccent1.Value );
                        }

                        bool isSelected = wasSelected;
                        if ( ImGui::Selectable( pComponent->GetNameID().c_str(), &isSelected ) )
                        {
                            if ( isSelected )
                            {
                                ClearSelection();
                                m_selectedComponents.clear();
                                m_selectedComponents.emplace_back( pComponent );

                                // Notify listeners that we want to edit the selected component
                                m_onRequestedTypeToEditChanged.Execute( pComponent );
                            }
                        }

                        if ( wasSelected )
                        {
                            ImGui::PopStyleColor();
                        }

                        ImGuiX::ItemTooltip( "Type: %s", pComponent->GetTypeInfo()->GetFriendlyTypeName() );

                        if ( ImGui::BeginPopupContextItem( "Component" ) )
                        {
                            if ( ImGui::MenuItem( EE_ICON_RENAME_BOX" Rename Component" ) )
                            {
                                StartOperation( Operation::RenameComponent, pComponent );
                            }

                            if ( ImGui::MenuItem( EE_ICON_DELETE" Remove Component" ) )
                            {
                                DestroyComponent( pComponent );
                            }

                            ImGui::EndPopup();
                        }
                        ImGui::PopID();
                    }

                    if ( m_pEditedEntity->GetComponents().empty() )
                    {
                        ImGui::Text( "No Components" );
                    }

                    ImGui::EndListBox();
                }

                //-------------------------------------------------------------------------
                // Systems
                //-------------------------------------------------------------------------

                ImGui::AlignTextToFramePadding();
                ImGui::Text( EE_ICON_COG_OUTLINE" Systems" );
                ImGui::SameLine( availableArea.x - 60 );
                {
                    ImGuiX::ScopedFont sf( ImGuiX::Font::SmallBold );
                    if ( ImGuiX::ColoredButton( ImGuiX::ImColors::Green, ImGuiX::ImColors::White, EE_ICON_COG_OUTLINE" ADD", addButtonSize ) )
                    {
                        StartOperation( Operation::AddSystem );
                    }
                    ImGuiX::ItemTooltip( "Add System" );
                }

                if ( ImGui::BeginListBox( "Systems", ImVec2( -1, listWidgetHeight ) ) )
                {
                    for ( auto pSystem : m_pEditedEntity->GetSystems() )
                    {
                        ImGui::PushID( pSystem );
                        if ( ImGui::Selectable( pSystem->GetTypeInfo()->GetFriendlyTypeName() ) )
                        {
                            ClearSelection();
                            m_onRequestedTypeToEditChanged.Execute( nullptr );
                        }

                        if ( ImGui::BeginPopupContextItem( "System" ) )
                        {
                            if ( ImGui::MenuItem( EE_ICON_DELETE" Remove System" ) )
                            {
                                DestroySystem( pSystem );
                            }

                            ImGui::EndPopup();
                        }
                        ImGui::PopID();
                    }

                    if ( m_pEditedEntity->GetSystems().empty() )
                    {
                        ImGui::Text( "No Systems" );
                    }

                    ImGui::EndListBox();
                }
            }
            else
            {
                if( m_shouldRefreshEditorState )
                {
                    // Display nothing
                }
                else if ( m_editedEntities.size() > 1 )
                {
                    ImGui::Text( "Multiple Entities Selected." );
                }
                else
                {
                    ImGui::Text( "Nothing to Edit." );
                }
            }

            isFocused = ImGui::IsWindowFocused( ImGuiFocusedFlags_ChildWindows );
        }
        ImGui::End();

        //-------------------------------------------------------------------------

        DrawDialogs();

        //-------------------------------------------------------------------------

        if ( isFocused )
        {
            if ( ImGui::IsKeyReleased( ImGuiKey_F2 ) )
            {
                if ( m_selectedComponents.size() == 1 )
                {
                    StartOperation( Operation::RenameComponent, m_selectedComponents[0] );
                }
                else if ( m_selectedSpatialComponents.size() == 1 )
                {
                    StartOperation( Operation::RenameComponent, m_selectedSpatialComponents[0] );
                }
            }
        }

        return isFocused;
    }

    void EntityStructureEditor::StartOperation( Operation operation, EntityComponent* pTargetComponent )
    {
        EE_ASSERT( m_pEditedEntity != nullptr );
        EE_ASSERT( operation != Operation::None );

        //-------------------------------------------------------------------------

        // If we are waiting for an operation to complete, we cant start a new one
        if ( m_pActiveUndoAction != nullptr )
        {
            return;
        }

        // Set operation common data
        //-------------------------------------------------------------------------

        m_activeOperation = operation;
        m_initializeFocus = true;
        m_pOperationTargetComponent = pTargetComponent;
        m_operationBuffer[0] = 0;
        m_operationOptions.clear();
        m_filteredOptions.clear();

        // Initialize operation custom data
        //-------------------------------------------------------------------------

        if ( operation == Operation::RenameComponent)
        {
            EE_ASSERT( m_pOperationTargetComponent != nullptr );
            Printf( m_operationBuffer, 256, m_pOperationTargetComponent->GetNameID().c_str() );
        }

        // Filter available selection options
        //-------------------------------------------------------------------------

        if ( operation == Operation::AddSystem )
        {
            TInlineVector<TypeSystem::TypeID, 10> restrictions;
            for ( auto pSystem : m_pEditedEntity->GetSystems() )
            {
                restrictions.emplace_back( pSystem->GetTypeID() );
            }

            //-------------------------------------------------------------------------

            for ( auto pSystemTypeInfo : m_allSystemTypes )
            {
                bool isValidOption = true;
                for ( auto pExistingSystem : m_pEditedEntity->GetSystems() )
                {
                    if ( m_pToolsContext->m_pTypeRegistry->AreTypesInTheSameHierarchy( pExistingSystem->GetTypeInfo(), pSystemTypeInfo ) )
                    {
                        isValidOption = false;
                        break;
                    }
                }

                if ( isValidOption )
                {
                    m_operationOptions.emplace_back( pSystemTypeInfo );
                }
            }
        }
        else if ( operation == Operation::AddComponent )
        {
            TInlineVector<TypeSystem::TypeID, 10> restrictions;
            for ( auto pComponent : m_pEditedEntity->GetComponents() )
            {
                if ( IsOfType<SpatialEntityComponent>( pComponent ) )
                {
                    continue;
                }

                if ( pComponent->IsSingletonComponent() )
                {
                    restrictions.emplace_back( pComponent->GetTypeID() );
                }
            }

            //-------------------------------------------------------------------------

            for ( auto pComponentTypeInfo : m_allComponentTypes )
            {
                bool isValidOption = true;
                for ( auto pExistingComponent : m_pEditedEntity->GetComponents() )
                {
                    if ( pExistingComponent->IsSingletonComponent() )
                    {
                        if ( m_pToolsContext->m_pTypeRegistry->AreTypesInTheSameHierarchy( pExistingComponent->GetTypeInfo(), pComponentTypeInfo ) )
                        {
                            isValidOption = false;
                            break;
                        }
                    }
                }

                if ( isValidOption )
                {
                    m_operationOptions.emplace_back( pComponentTypeInfo );
                }
            }
        }
        else if ( operation == Operation::AddSpatialComponent )
        {
            if ( m_pOperationTargetComponent != nullptr )
            {
                EE_ASSERT( IsOfType<SpatialEntityComponent>( m_pOperationTargetComponent ) );
            }

            //-------------------------------------------------------------------------

            TInlineVector<TypeSystem::TypeID, 10> restrictions;
            for ( auto pComponent : m_pEditedEntity->GetComponents() )
            {
                if ( !IsOfType<SpatialEntityComponent>( pComponent ) )
                {
                    continue;
                }

                if ( pComponent->IsSingletonComponent() )
                {
                    restrictions.emplace_back( pComponent->GetTypeID() );
                }
            }

            //-------------------------------------------------------------------------

            for ( auto pComponentTypeInfo : m_allSpatialComponentTypes )
            {
                bool isValidOption = true;
                for ( auto pExistingComponent : m_pEditedEntity->GetComponents() )
                {
                    if ( pExistingComponent->IsSingletonComponent() )
                    {
                        if ( m_pToolsContext->m_pTypeRegistry->AreTypesInTheSameHierarchy( pExistingComponent->GetTypeInfo(), pComponentTypeInfo ) )
                        {
                            isValidOption = false;
                            break;
                        }
                    }
                }

                if ( isValidOption )
                {
                    m_operationOptions.emplace_back( pComponentTypeInfo );
                }
            }
        }

        if ( m_activeOperation != Operation::RenameComponent )
        {
            m_pOperationSelectedOption = m_operationOptions.front();
            m_filteredOptions = m_operationOptions;
        }
    }

    void EntityStructureEditor::DrawDialogs()
    {
        constexpr static const char* const dialogTitles[3] = { "Add System##ASC", "Add Spatial Component##ASC", "Add Component##ASC" };

        bool isDialogOpen = m_activeOperation != Operation::None;
        bool completeOperation = false;
        if ( isDialogOpen )
        {
            if ( m_activeOperation == Operation::RenameComponent )
            {
                EE_ASSERT( m_pEditedEntity != nullptr && m_pOperationTargetComponent != nullptr );

                //-------------------------------------------------------------------------

                auto ValidateName = [this] ()
                {
                    bool isValidName = strlen( m_operationBuffer ) > 0;
                    if ( isValidName )
                    {
                        StringID const desiredNameID = StringID( m_operationBuffer );
                        auto uniqueNameID = m_pEditedEntity->GenerateUniqueComponentNameID( m_pOperationTargetComponent, desiredNameID );
                        isValidName = ( desiredNameID == uniqueNameID );
                    }

                    return isValidName;
                };

                constexpr static char const* const dialogName = "Rename Entity";
                ImGui::OpenPopup( dialogName );
                ImGui::SetNextWindowSize( ImVec2( 400, 90 ) );
                if ( ImGui::BeginPopupModal( dialogName, &isDialogOpen, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar ) )
                {
                    bool applyRename = false;
                    bool isValidName = ValidateName();

                    // Draw UI
                    //-------------------------------------------------------------------------

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text( "Name: " );
                    ImGui::SameLine();

                    ImGui::SetNextItemWidth( -1 );
                    ImGui::PushStyleColor( ImGuiCol_Text, isValidName ? (uint32_t) ImGuiX::Style::s_colorText : Colors::Red.ToUInt32_ABGR() );
                    applyRename = ImGui::InputText( "##Name", m_operationBuffer, 256, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CallbackCharFilter, ImGuiX::FilterNameIDChars );
                    isValidName = ValidateName();
                    ImGui::PopStyleColor();

                    ImGui::NewLine();
                    ImGui::SameLine( ImGui::GetContentRegionAvail().x - 120 - ImGui::GetStyle().ItemSpacing.x );

                    ImGui::BeginDisabled( !isValidName );
                    if ( ImGui::Button( "OK", ImVec2( 60, 0 ) ) )
                    {
                        applyRename = true;
                    }
                    ImGui::EndDisabled();

                    ImGui::SameLine();
                    if ( ImGui::Button( "Cancel", ImVec2( 60, 0 ) ) )
                    {
                        m_activeOperation = Operation::None;
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();

                    //-------------------------------------------------------------------------

                    if ( applyRename && isValidName )
                    {
                        StringID const desiredNameID = StringID( m_operationBuffer );
                        RenameComponent( m_pOperationTargetComponent, desiredNameID );
                        m_activeOperation = Operation::None;
                    }
                }
            }
            else // Add Component/System
            {
                ImGui::OpenPopup( dialogTitles[(int32_t) m_activeOperation] );
                ImGui::SetNextWindowSize( ImVec2( 1000, 400 ), ImGuiCond_FirstUseEver );
                ImGui::SetNextWindowSizeConstraints( ImVec2( 400, 400 ), ImVec2( FLT_MAX, FLT_MAX ) );
                if ( ImGui::BeginPopupModal( dialogTitles[(int32_t) m_activeOperation], &isDialogOpen, ImGuiWindowFlags_NoNavInputs ) )
                {
                    ImVec2 const contentRegionAvailable = ImGui::GetContentRegionAvail();

                    // Draw Filter
                    //-------------------------------------------------------------------------

                    bool filterUpdated = false;

                    ImGui::SetNextItemWidth( contentRegionAvailable.x - ImGui::GetStyle().WindowPadding.x - 26 );
                    InlineString filterCopy( m_operationBuffer );

                    if ( m_initializeFocus )
                    {
                        ImGui::SetKeyboardFocusHere();
                        m_initializeFocus = false;
                    }

                    if ( ImGui::InputText( "##Filter", filterCopy.data(), 256 ) )
                    {
                        if ( strcmp( filterCopy.data(), m_operationBuffer ) != 0 )
                        {
                            strcpy_s( m_operationBuffer, 256, filterCopy.data() );

                            // Convert buffer to lower case
                            int32_t i = 0;
                            while ( i < 256 && m_operationBuffer[i] != 0 )
                            {
                                m_operationBuffer[i] = eastl::CharToLower( m_operationBuffer[i] );
                                i++;
                            }

                            filterUpdated = true;
                        }
                    }

                    ImGui::SameLine();
                    if ( ImGui::Button( EE_ICON_CLOSE_CIRCLE "##Clear Filter", ImVec2( 26, 0 ) ) )
                    {
                        m_operationBuffer[0] = 0;
                        filterUpdated = true;
                    }

                    // Update filter options
                    //-------------------------------------------------------------------------

                    if ( filterUpdated )
                    {
                        if ( m_operationBuffer[0] == 0 )
                        {
                            m_filteredOptions = m_operationOptions;
                        }
                        else
                        {
                            m_filteredOptions.clear();
                            for ( auto const& pTypeInfo : m_operationOptions )
                            {
                                String lowercasePath( pTypeInfo->GetTypeName() );
                                lowercasePath.make_lower();

                                bool passesFilter = true;
                                char* token = strtok( m_operationBuffer, " " );
                                while ( token )
                                {
                                    if ( lowercasePath.find( token ) == String::npos )
                                    {
                                        passesFilter = false;
                                        break;
                                    }

                                    token = strtok( NULL, " " );
                                }

                                if ( passesFilter )
                                {
                                    m_filteredOptions.emplace_back( pTypeInfo );
                                }
                            }
                        }

                        m_pOperationSelectedOption = m_filteredOptions.empty() ? nullptr : m_filteredOptions.front();
                    }

                    // Draw results
                    //-------------------------------------------------------------------------

                    float const tableHeight = contentRegionAvailable.y - ImGui::GetFrameHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.y;
                    ImGui::PushStyleColor( ImGuiCol_Header, ImGuiX::Style::s_colorGray1.Value );
                    if ( ImGui::BeginTable( "Options List", 1, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2( contentRegionAvailable.x, tableHeight ) ) )
                    {
                        ImGui::TableSetupColumn( "Type", ImGuiTableColumnFlags_WidthStretch, 1.0f );

                        //-------------------------------------------------------------------------

                        ImGuiListClipper clipper;
                        clipper.Begin( (int32_t) m_filteredOptions.size() );
                        while ( clipper.Step() )
                        {
                            for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                            {
                                ImGui::TableNextRow();

                                ImGui::TableNextColumn();
                                bool const wasSelected = ( m_pOperationSelectedOption == m_filteredOptions[i] );
                                if ( wasSelected )
                                {
                                    ImGui::PushStyleColor( ImGuiCol_Text, ImGuiX::Style::s_colorAccent0.Value );
                                }

                                bool isSelected = wasSelected;
                                if ( ImGui::Selectable( m_filteredOptions[i]->GetFriendlyTypeName(), &isSelected, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns ) )
                                {
                                    m_pOperationSelectedOption = m_filteredOptions[i];

                                    if ( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
                                    {
                                        completeOperation = true;
                                    }
                                }

                                if ( wasSelected )
                                {
                                    ImGui::PopStyleColor();
                                }
                            }
                        }

                        ImGui::EndTable();
                    }
                    ImGui::PopStyleColor();

                    //-------------------------------------------------------------------------

                    auto AdjustSelection = [this] ( bool increment )
                    {
                        int32_t const numOptions = (int32_t) m_filteredOptions.size();
                        if ( numOptions == 0 )
                        {
                            return;
                        }

                        int32_t optionIdx = InvalidIndex;
                        for ( auto i = 0; i < numOptions; i++ )
                        {
                            if ( m_filteredOptions[i] == m_pOperationSelectedOption )
                            {
                                optionIdx = i;
                                break;
                            }
                        }

                        EE_ASSERT( optionIdx != InvalidIndex );

                        optionIdx += ( increment ? 1 : -1 );
                        if ( optionIdx < 0 )
                        {
                            optionIdx += numOptions;
                        }
                        optionIdx = optionIdx % m_filteredOptions.size();
                        m_pOperationSelectedOption = m_filteredOptions[optionIdx];
                    };

                    if ( ImGui::IsKeyReleased( ImGuiKey_Enter ) )
                    {
                        completeOperation = true;
                    }
                    else if ( ImGui::IsKeyReleased( ImGuiKey_Escape ) )
                    {
                        ImGui::CloseCurrentPopup();
                        isDialogOpen = false;
                    }
                    else if ( ImGui::IsKeyReleased( ImGuiKey_UpArrow ) )
                    {
                        if ( m_filteredOptions.size() > 0 )
                        {
                            AdjustSelection( false );
                            m_initializeFocus = true;
                        }
                    }
                    else if ( ImGui::IsKeyReleased( ImGuiKey_DownArrow ) )
                    {
                        if ( m_filteredOptions.size() > 0 )
                        {
                            AdjustSelection( true );
                            m_initializeFocus = true;
                        }
                    }

                    //-------------------------------------------------------------------------

                    if ( completeOperation && m_pOperationSelectedOption == nullptr )
                    {
                        completeOperation = false;
                    }

                    if ( completeOperation )
                    {
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();

                    //-------------------------------------------------------------------------

                    if ( completeOperation )
                    {
                        EE_ASSERT( m_activeOperation != Operation::None );
                        EE_ASSERT( m_pOperationSelectedOption != nullptr );

                        if ( m_activeOperation == Operation::AddSystem )
                        {
                            CreateSystem( m_pOperationSelectedOption );
                        }
                        else if ( m_activeOperation == Operation::AddComponent )
                        {
                            CreateComponent( m_pOperationSelectedOption );
                        }
                        else if ( m_activeOperation == Operation::AddSpatialComponent )
                        {
                            ComponentID parentComponentID;
                            if ( m_pOperationTargetComponent != nullptr )
                            {
                                EE_ASSERT( IsOfType<SpatialEntityComponent>( m_pOperationTargetComponent ) );
                                parentComponentID = m_pOperationTargetComponent->GetID();
                            }
                            CreateComponent( m_pOperationSelectedOption, parentComponentID );
                        }

                        //-------------------------------------------------------------------------

                        m_activeOperation = Operation::None;
                        m_pOperationSelectedOption = nullptr;
                        m_pOperationTargetComponent = nullptr;
                    }
                }
            }
        }

        //-------------------------------------------------------------------------

        // If the dialog was closed (i.e. operation canceled)
        if ( !isDialogOpen )
        {
            m_activeOperation = Operation::None;
        }
    }

    //-------------------------------------------------------------------------
    // Tree View
    //-------------------------------------------------------------------------

    static void FillTree( SpatialComponentItem* pParentItem, TInlineVector<SpatialEntityComponent*, 10> const& spatialComponents )
    {
        EE_ASSERT( pParentItem != nullptr );

        for ( auto i = 0u; i < spatialComponents.size(); i++ )
        {
            if ( spatialComponents[i]->GetSpatialParentID() == pParentItem->m_pComponent->GetID() )
            {
                auto pNewParentSpatialComponent = pParentItem->CreateChild<SpatialComponentItem>( spatialComponents[i] );
                pNewParentSpatialComponent->SetExpanded( true );
                FillTree( pNewParentSpatialComponent, spatialComponents );
            }
        }
    }

    void EntityStructureEditor::RebuildTreeUserFunction()
    {
        if ( m_pEditedEntity == nullptr )
        {
            return;
        }

        if ( m_pEditedEntity->HasStateChangeActionsPending() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        TInlineVector<SpatialEntityComponent*, 10> spatialComponents;

        for ( auto pComponent : m_pEditedEntity->GetComponents() )
        {
            if ( auto pSpatialComponent = TryCast<SpatialEntityComponent>( pComponent ) )
            {
                spatialComponents.emplace_back( pSpatialComponent );
            }
        }

        if ( !spatialComponents.empty() )
        {
            auto pParentSpatialComponent = m_rootItem.CreateChild<SpatialComponentItem>( m_pEditedEntity->GetRootSpatialComponent() );
            pParentSpatialComponent->SetExpanded( true );
            spatialComponents.erase_first_unsorted( m_pEditedEntity->GetRootSpatialComponent() );
            FillTree( pParentSpatialComponent, spatialComponents );
        }
    }

    void EntityStructureEditor::DrawItemContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus )
    {
        auto pItem = static_cast<SpatialComponentItem*>( selectedItemsWithContextMenus[0] );

        if ( ImGui::MenuItem( EE_ICON_RENAME_BOX" Rename Component" ) )
        {
            StartOperation( Operation::RenameComponent, pItem->m_pComponent );
        }

        if ( ImGui::MenuItem( EE_ICON_PLUS" Add Child Component" ) )
        {
            StartOperation( Operation::AddSpatialComponent, pItem->m_pComponent );
        }

        //-------------------------------------------------------------------------

        bool const isRootComponent = pItem->m_pComponent->IsRootComponent();

        if ( !isRootComponent )
        {
            if ( ImGui::MenuItem( EE_ICON_STAR" Make Root Component" ) )
            {
                MakeRootComponent( pItem->m_pComponent );
            }
        }

        // Only allow removal of the root if we can easily replace it
        if ( !isRootComponent || pItem->GetChildren().size() <= 1 )
        {
            if ( ImGui::MenuItem( EE_ICON_DELETE" Remove Component" ) )
            {
                DestroyComponent( pItem->m_pComponent );
            }
        }
    }

    void EntityStructureEditor::HandleSelectionChanged( TreeListView::ChangeReason reason )
    {
        m_selectedSpatialComponents.clear();

        auto const& selection = GetSelection();
        for( auto pSelectedItem : selection )
        {
            auto pSCItem = static_cast<SpatialComponentItem*>( pSelectedItem );
            m_selectedSpatialComponents.emplace_back( pSCItem->m_pComponent );
        }

        // If we have selected spatial components, clear the selected components
        if ( !m_selectedSpatialComponents.empty() )
        {
            m_selectedComponents.clear();
        }

        // Notify listeners that we want to edit the selected component
        if ( reason != TreeListView::ChangeReason::TreeRebuild )
        {
            if ( m_selectedSpatialComponents.empty() )
            {
                m_onRequestedTypeToEditChanged.Execute( nullptr );
            }
            else
            {
                m_onRequestedTypeToEditChanged.Execute( m_selectedSpatialComponents.back() );
            }
        }
    }

    void EntityStructureEditor::HandleDragAndDropOnItem( TreeListViewItem* pDragAndDropTargetItem ) 
    {
        if ( ImGuiPayload const* payload = ImGui::AcceptDragDropPayload( SpatialComponentItem::s_dragAndDropID, ImGuiDragDropFlags_AcceptBeforeDelivery ) )
        {
            if ( payload->IsDelivery() )
            {
                auto pRawData = (uintptr_t*) payload->Data;
                auto pSourceComponentItem = (SpatialComponentItem*) *pRawData;
                auto pTargetComponentItem = (SpatialComponentItem*) pDragAndDropTargetItem;

                // Same items, nothing to do
                if ( pSourceComponentItem == pTargetComponentItem )
                {
                    return;
                }

                // We cannot re-parent ourselves to one of our children
                if ( pTargetComponentItem->m_pComponent->IsSpatialChildOf( pSourceComponentItem->m_pComponent ) )
                {
                    return;
                }

                // Perform operation
                ReparentComponent( pSourceComponentItem->m_pComponent, pTargetComponentItem->m_pComponent );
            }
        }
    }

    //-------------------------------------------------------------------------
    // Entity Operations
    //-------------------------------------------------------------------------

    void EntityStructureEditor::CreateSystem( TypeSystem::TypeInfo const* pSystemTypeInfo )
    {
        EE_ASSERT( pSystemTypeInfo != nullptr );
        EE_ASSERT( m_pActiveUndoAction == nullptr );
        EE_ASSERT( m_pEditedEntity != nullptr );
        EE_ASSERT( m_pEditedEntity->IsAddedToMap() );

        m_pActiveUndoAction = EE::New<EntityUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pWorld );
        m_pActiveUndoAction->RecordBeginEdit( { m_pEditedEntity } );
        m_pEditedEntity->CreateSystem( pSystemTypeInfo );

        m_pActiveUndoActionEntity = m_pEditedEntity;
    }

    void EntityStructureEditor::DestroySystem( TypeSystem::TypeID systemTypeID )
    {
        EE_ASSERT( systemTypeID.IsValid() );
        EE_ASSERT( m_pEditedEntity != nullptr );
        EE_ASSERT( m_pActiveUndoAction == nullptr );

        m_pActiveUndoAction = EE::New<EntityUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pWorld );
        m_pActiveUndoAction->RecordBeginEdit( { m_pEditedEntity } );
        m_pEditedEntity->DestroySystem( systemTypeID );

        m_pActiveUndoActionEntity = m_pEditedEntity;
    }

    void EntityStructureEditor::DestroySystem( EntitySystem* pSystem )
    {
        EE_ASSERT( pSystem != nullptr );
        EE_ASSERT( m_pEditedEntity != nullptr );
        EE_ASSERT( m_pActiveUndoAction == nullptr );
        EE_ASSERT( VectorContains( m_pEditedEntity->GetSystems(), pSystem ) );

        m_pActiveUndoAction = EE::New<EntityUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pWorld );
        m_pActiveUndoAction->RecordBeginEdit( { m_pEditedEntity } );
        m_pEditedEntity->DestroySystem( pSystem->GetTypeID() );

        m_pActiveUndoActionEntity = m_pEditedEntity;
    }

    void EntityStructureEditor::CreateComponent( TypeSystem::TypeInfo const* pComponentTypeInfo, ComponentID const& parentSpatialComponentID )
    {
        EE_ASSERT( pComponentTypeInfo != nullptr );
        EE_ASSERT( m_pEditedEntity != nullptr );
        EE_ASSERT( m_pActiveUndoAction == nullptr );

        m_pActiveUndoAction = EE::New<EntityUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pWorld );
        m_pActiveUndoAction->RecordBeginEdit( { m_pEditedEntity } );
        m_pEditedEntity->CreateComponent( pComponentTypeInfo, parentSpatialComponentID );

        m_pActiveUndoActionEntity = m_pEditedEntity;
    }

    void EntityStructureEditor::DestroyComponent( EntityComponent* pComponent )
    {
        EE_ASSERT( pComponent != nullptr );
        EE_ASSERT( m_pEditedEntity != nullptr );
        EE_ASSERT( m_pActiveUndoAction == nullptr );

        m_pActiveUndoAction = EE::New<EntityUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pWorld );
        m_pActiveUndoAction->RecordBeginEdit( { m_pEditedEntity } );
        m_pEditedEntity->DestroyComponent( pComponent->GetID() );

        // Check if there are any other spatial components left
        if ( m_pEditedEntity->IsSpatialEntity() && IsOfType<SpatialEntityComponent>( pComponent ) )
        {
            bool isStillASpatialEntity = false;
            for ( auto pExistingComponent : m_pEditedEntity->m_components )
            {
                if ( pExistingComponent == pComponent )
                {
                    continue;
                }

                if ( IsOfType<SpatialEntityComponent>( pExistingComponent ) )
                {
                    isStillASpatialEntity = true;
                    break;
                }
            }
            // If we are no longer a spatial entity, clear the parent
            if ( !isStillASpatialEntity && m_pEditedEntity->HasSpatialParent() )
            {
                m_pEditedEntity->ClearSpatialParent();
            }
        }

        m_pActiveUndoActionEntity = m_pEditedEntity;
    }

    void EntityStructureEditor::RenameComponent( EntityComponent* pComponent, StringID newNameID )
    {
        EE_ASSERT( pComponent != nullptr );
        EE_ASSERT( m_pEditedEntity != nullptr );
        EE_ASSERT( m_pActiveUndoAction == nullptr );

        //-------------------------------------------------------------------------

        m_pActiveUndoAction = EE::New<EntityUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pWorld );
        m_pActiveUndoAction->RecordBeginEdit( { m_pEditedEntity } );

        //-------------------------------------------------------------------------

        m_pEditedEntity->RenameComponent( pComponent, newNameID );

        //-------------------------------------------------------------------------
        m_pActiveUndoAction->RecordEndEdit();
        m_pUndoStack->RegisterAction( m_pActiveUndoAction );
        m_pActiveUndoAction = nullptr;

        m_shouldRefreshEditorState = true;
    }

    void EntityStructureEditor::ReparentComponent( SpatialEntityComponent* pComponent, SpatialEntityComponent* pNewParentComponent )
    {
        EE_ASSERT( m_pEditedEntity != nullptr );
        EE_ASSERT( m_pActiveUndoAction == nullptr );
        EE_ASSERT( pComponent != nullptr && pNewParentComponent != nullptr );
        EE_ASSERT( !pComponent->IsRootComponent() );

        // Nothing to do, the new parent is the current parent
        if ( pComponent->HasSpatialParent() && pComponent->GetSpatialParentID() == pNewParentComponent->GetID() )
        {
            return;
        }

        // Create undo action
        m_pActiveUndoAction = EE::New<EntityUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pWorld );
        m_pActiveUndoAction->RecordBeginEdit( { m_pEditedEntity } );

        //-------------------------------------------------------------------------

        // Flag all components for edit
        for ( auto pExistingComponent : m_pEditedEntity->GetComponents() )
        {
            m_pWorld->BeginComponentEdit( pExistingComponent );
        }

        // Remove the component from its old parent
        pComponent->m_pSpatialParent->m_spatialChildren.erase_first( pComponent );
        pComponent->m_pSpatialParent = nullptr;

        // Add the component to its new parent
        pNewParentComponent->m_spatialChildren.emplace_back( pComponent );
        pComponent->m_pSpatialParent = pNewParentComponent;

        // Complete Component edits
        for ( auto pExistingComponent : m_pEditedEntity->GetComponents() )
        {
            m_pWorld->EndComponentEdit( pExistingComponent );
        }

        //-------------------------------------------------------------------------

        m_pActiveUndoActionEntity = m_pEditedEntity;
    }

    void EntityStructureEditor::MakeRootComponent( SpatialEntityComponent* pComponent )
    {
        EE_ASSERT( m_pEditedEntity != nullptr );
        EE_ASSERT( m_pActiveUndoAction == nullptr );
        EE_ASSERT( pComponent != nullptr );

        // This component is already the root
        if ( m_pEditedEntity->GetRootSpatialComponentID() == pComponent->GetID() )
        {
            return;
        }

        // Create undo action
        m_pActiveUndoAction = EE::New<EntityUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pWorld );
        m_pActiveUndoAction->RecordBeginEdit( { m_pEditedEntity } );

        //-------------------------------------------------------------------------

        // Break any cross entity links
        bool recreateSpatialAttachment = m_pEditedEntity->m_isSpatialAttachmentCreated;
        if ( recreateSpatialAttachment )
        {
            m_pEditedEntity->DestroySpatialAttachment( Entity::SpatialAttachmentRule::KeepLocalTranform );
        }

        // Flag all components for edit
        for ( auto pExistingComponent : m_pEditedEntity->GetComponents() )
        {
            m_pWorld->BeginComponentEdit( pExistingComponent );
        }

        // Remove the component from its parent
        pComponent->m_pSpatialParent->m_spatialChildren.erase_first( pComponent );
        pComponent->m_pSpatialParent = nullptr;

        // Add the old root component as a child of the new root
        pComponent->m_spatialChildren.emplace_back( m_pEditedEntity->m_pRootSpatialComponent );
        m_pEditedEntity->m_pRootSpatialComponent->m_pSpatialParent = pComponent;

        // Make the new component the root
        m_pEditedEntity->m_pRootSpatialComponent = pComponent;

        // Recreate the cross entity links
        if ( recreateSpatialAttachment )
        {
            m_pEditedEntity->CreateSpatialAttachment();
        }

        // End components edit operations
        for ( auto pExistingComponent : m_pEditedEntity->GetComponents() )
        {
            m_pWorld->EndComponentEdit( pExistingComponent );
        }

        //-------------------------------------------------------------------------

        m_pActiveUndoActionEntity = m_pEditedEntity;
    }
}