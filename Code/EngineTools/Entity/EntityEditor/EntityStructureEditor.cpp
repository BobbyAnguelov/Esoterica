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
    class StructureEditorItem : public TreeListViewItem
    {

    public:

        constexpr static char const* const s_dragAndDropID = "SpatialComponentItem";
        constexpr static char const* const s_spatialComponentsHeaderID = EE_ICON_AXIS_ARROW" Spatial Components";
        constexpr static char const* const s_componentsHeaderID = EE_ICON_PACKAGE_VARIANT_CLOSED" Components";
        constexpr static char const* const s_systemsHeaderID = EE_ICON_PROGRESS_WRENCH" Systems";

    public:

        StructureEditorItem( char const* const pLabel )
            : TreeListViewItem()
            , m_ID( pLabel )
            , m_tooltip( pLabel )
        {
            SetExpanded( true );
        }

        explicit StructureEditorItem( SpatialEntityComponent* pComponent )
            : TreeListViewItem()
            , m_ID( pComponent->GetNameID() )
            , m_pComponent( pComponent )
            , m_pSpatialComponent( pComponent )
            , m_componentID( pComponent->GetID() )
        {
            m_tooltip.sprintf( "Type: %s", m_pComponent->GetTypeInfo()->GetFriendlyTypeName() );
            SetExpanded( true );
        }

        StructureEditorItem( EntityComponent* pComponent )
            : TreeListViewItem()
            , m_ID( pComponent->GetNameID() )
            , m_pComponent( pComponent )
            , m_componentID( pComponent->GetID() )
        {
            EE_ASSERT( !IsOfType<SpatialEntityComponent>( pComponent ) );
            m_tooltip.sprintf( "Type: %s", m_pComponent->GetTypeInfo()->GetFriendlyTypeName() );
        }

        StructureEditorItem( EntitySystem* pSystem )
            : TreeListViewItem()
            , m_ID( pSystem->GetTypeInfo()->GetFriendlyTypeName() )
            , m_pSystem( pSystem )
        {
            EE_ASSERT( pSystem != nullptr );
            m_tooltip.sprintf( "Type: %s", m_pSystem->GetTypeInfo()->GetFriendlyTypeName() );
        }

        //-------------------------------------------------------------------------

        virtual bool IsHeader() const { return m_pComponent == nullptr && m_pSystem == nullptr; }
        bool IsComponent() const { return m_pComponent != nullptr; }
        bool IsSpatialComponent() const { return m_pComponent != nullptr && m_pSpatialComponent != nullptr; }
        bool IsSystem() const { return m_pSystem == nullptr; }

        //-------------------------------------------------------------------------

        virtual StringID GetNameID() const override { return m_ID; }

        virtual uint64_t GetUniqueID() const override
        {
            if ( IsComponent() )
            {
                return m_componentID.m_value;
            }

            return m_ID.GetID();
        }

        virtual bool HasContextMenu() const override { return true; }

        virtual char const* GetTooltipText() const override { return m_tooltip.c_str(); }
        
        virtual bool IsDragAndDropSource() const override { return IsSpatialComponent() && !m_pSpatialComponent->IsRootComponent(); }
        
        virtual bool IsDragAndDropTarget() const override { return IsSpatialComponent(); }

        virtual void SetDragAndDropPayloadData() const override
        {
            EE_ASSERT( IsDragAndDropSource() );
            uintptr_t itemAddress = uintptr_t( this );
            ImGui::SetDragDropPayload( s_dragAndDropID, &itemAddress, sizeof( uintptr_t ) );
        }

    public:

        StringID                    m_ID;
        String                      m_tooltip;

        // Components - if this is a component, the component ptr must be set, the spatial component ptr is optionally set for spatial components
        EntityComponent*            m_pComponent = nullptr;
        SpatialEntityComponent*     m_pSpatialComponent = nullptr;
        ComponentID                 m_componentID; // Cached since we want to access this on rebuild without touching the component memory which might have been invalidated

        // Systems
        EntitySystem*               m_pSystem = nullptr;
    };
}

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    using namespace TypeSystem;

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
        m_prioritizeBranchesOverLeavesInVisualTree = false;

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

    void EntityStructureEditor::Initialize( UpdateContext const& context, uint32_t widgetUniqueID )
    {
        m_windowName.sprintf( "Entity##%u", widgetUniqueID );
        m_entityStateChangedBindingID = Entity::OnEntityUpdated().Bind( [this] ( Entity* pEntityChanged ) { EntityUpdateDetected( pEntityChanged ); } );
    }

    void EntityStructureEditor::Shutdown( UpdateContext const& context )
    {
        Entity::OnEntityUpdated().Unbind( m_entityStateChangedBindingID );
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

                float const editButtonWidth = 26;
                float const addButtonWidth = 60;
                float const buttonHeight = ImGui::GetFrameHeight();
                float const spacing = ImGui::GetStyle().ItemSpacing.x;

                {
                    ImGuiX::ScopedFont sf( ImGuiX::Font::MediumBold );
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text( "Entity: %s", m_pEditedEntity->GetNameID().c_str() );
                }

                ImGui::SameLine( ImGui::GetContentRegionAvail().x - ( addButtonWidth + editButtonWidth + spacing ), 0);
                if ( ImGui::Button( EE_ICON_PLAYLIST_EDIT, ImVec2( editButtonWidth, buttonHeight ) ) )
                {
                    ClearSelection();
                    m_onRequestedTypeToEditChanged.Execute( m_pEditedEntity );
                }
                ImGuiX::ItemTooltip( "Edit Entity Details" );

                ImGui::SameLine();

                ImGuiX::ScopedFont sf( ImGuiX::Font::SmallBold );
                if ( ImGuiX::ColoredButton( ImGuiX::ImColors::Green, ImGuiX::ImColors::White, EE_ICON_PLUS" ADD", ImVec2( addButtonWidth, buttonHeight ) ) )
                {
                    m_pOperationTargetComponent = nullptr;
                    StartOperation( Operation::AddAny );
                }
                ImGuiX::ItemTooltip( "Add Component/System" );

                ImGui::Separator();

                //-------------------------------------------------------------------------
                // Entity Tree
                //-------------------------------------------------------------------------

                auto const& style = ImGui::GetStyle();
                TreeListView::UpdateAndDraw( -1.0f );
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

        auto AddAvailableSystemOptions = [this] ()
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
        };

        auto AddAvailableComponentOptions = [this] ()
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
        };

        auto AddAvailableSpatialComponentOptions = [this] ()
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
        };

        if ( operation == Operation::AddAny )
        {
            AddAvailableSystemOptions();
            AddAvailableComponentOptions();
            AddAvailableSpatialComponentOptions();
        }
        else if ( operation == Operation::AddSystem )
        {
            AddAvailableSystemOptions();
        }
        else if ( operation == Operation::AddComponent )
        {
            AddAvailableComponentOptions();
        }
        else if ( operation == Operation::AddSpatialComponent )
        {
            AddAvailableSpatialComponentOptions();
        }

        //-------------------------------------------------------------------------

        if ( m_activeOperation != Operation::RenameComponent )
        {
            m_pOperationSelectedOption = m_operationOptions.front();
            m_filteredOptions = m_operationOptions;
        }
    }

    void EntityStructureEditor::DrawDialogs()
    {
        constexpr static const char* const dialogTitles[4] = {  "Add Item##ASC", "Add System##ASC", "Add Spatial Component##ASC", "Add Component##ASC" };

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

                        if ( VectorContains( m_allSystemTypes, m_pOperationSelectedOption ) )
                        {
                            CreateSystem( m_pOperationSelectedOption );
                        }
                        else if ( VectorContains( m_allComponentTypes, m_pOperationSelectedOption ) )
                        {
                            CreateComponent( m_pOperationSelectedOption );
                        }
                        else if ( VectorContains( m_allSpatialComponentTypes, m_pOperationSelectedOption ) )
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

    static void FillTree( StructureEditorItem* pParentItem, TInlineVector<SpatialEntityComponent*, 10> const& spatialComponents )
    {
        EE_ASSERT( pParentItem != nullptr && pParentItem->IsSpatialComponent() );

        for ( auto i = 0u; i < spatialComponents.size(); i++ )
        {
            if ( !spatialComponents[i]->HasSpatialParent() )
            {
                continue;
            }

            if ( spatialComponents[i]->GetSpatialParentID() == pParentItem->m_pComponent->GetID() )
            {
                auto pNewParentSpatialComponent = pParentItem->CreateChild<StructureEditorItem>( spatialComponents[i] );
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

        // Split Components
        //-------------------------------------------------------------------------

        TInlineVector<SpatialEntityComponent*, 10> spatialComponents;
        TInlineVector<EntityComponent*, 10> nonSpatialComponents;

        for ( auto pComponent : m_pEditedEntity->GetComponents() )
        {
            if ( auto pSpatialComponent = TryCast<SpatialEntityComponent>( pComponent ) )
            {
                spatialComponents.emplace_back( pSpatialComponent );
            }
            else
            {
                nonSpatialComponents.emplace_back( pComponent );
            }
        }

        // Spatial Components
        //-------------------------------------------------------------------------

        auto pSpatialComponentsHeaderItem = m_rootItem.CreateChild<StructureEditorItem>( StructureEditorItem::s_spatialComponentsHeaderID );
        if( m_pEditedEntity->IsSpatialEntity() )
        {
            auto pRootComponentItem = pSpatialComponentsHeaderItem->CreateChild<StructureEditorItem>( m_pEditedEntity->GetRootSpatialComponent() );
            FillTree( pRootComponentItem, spatialComponents );
        }

        SortItemChildren( pSpatialComponentsHeaderItem );

        // Non-Spatial Components
        //-------------------------------------------------------------------------

        auto pComponentsHeaderItem = m_rootItem.CreateChild<StructureEditorItem>( StructureEditorItem::s_componentsHeaderID );
        for ( auto pComponent : nonSpatialComponents )
        {
            pComponentsHeaderItem->CreateChild<StructureEditorItem>( pComponent );
        }

        SortItemChildren( pComponentsHeaderItem );

        // Systems
        //-------------------------------------------------------------------------

        auto pSystemsHeaderItem = m_rootItem.CreateChild<StructureEditorItem>( StructureEditorItem::s_systemsHeaderID );
        for ( auto pSystem : m_pEditedEntity->GetSystems() )
        {
            pSystemsHeaderItem->CreateChild<StructureEditorItem>( pSystem );
        }

        SortItemChildren( pSystemsHeaderItem );
    }

    void EntityStructureEditor::DrawItemContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus )
    {
        auto pItem = static_cast<StructureEditorItem const*>( selectedItemsWithContextMenus[0] );

        //-------------------------------------------------------------------------
        // Header Context Menu
        //-------------------------------------------------------------------------
        if ( pItem->IsHeader() )
        {
            if ( pItem->m_ID == StringID( StructureEditorItem::s_spatialComponentsHeaderID ) )
            {
                if ( ImGui::MenuItem( EE_ICON_PLUS" Add Component" ) )
                {
                    StartOperation( Operation::AddSpatialComponent );
                }
            }
            else if ( pItem->m_ID == StringID( StructureEditorItem::s_componentsHeaderID ) )
            {
                if ( ImGui::MenuItem( EE_ICON_PLUS" Add Component" ) )
                {
                    StartOperation( Operation::AddComponent, pItem->m_pComponent );
                }
            }
            else if ( pItem->m_ID == StringID( StructureEditorItem::s_systemsHeaderID ) )
            {
                if ( ImGui::MenuItem( EE_ICON_PLUS" Add System" ) )
                {
                    StartOperation( Operation::AddSystem, pItem->m_pComponent );
                }
            }
        }
        //-------------------------------------------------------------------------
        // Component Context Menu
        //-------------------------------------------------------------------------
        else if ( pItem->IsComponent() )
        {
            bool canRemoveItem = true;

            if ( ImGui::MenuItem( EE_ICON_RENAME_BOX" Rename Component" ) )
            {
                StartOperation( Operation::RenameComponent, pItem->m_pComponent );
            }

            //-------------------------------------------------------------------------

            if ( pItem->IsSpatialComponent() )
            {
                bool const isRootComponent = pItem->m_pSpatialComponent->IsRootComponent();
                if ( !isRootComponent )
                {
                    if ( ImGui::MenuItem( EE_ICON_STAR" Make Root Component" ) )
                    {
                        MakeRootComponent( pItem->m_pSpatialComponent );
                    }
                }

                if ( ImGui::MenuItem( EE_ICON_PLUS" Add Child Component" ) )
                {
                    StartOperation( Operation::AddSpatialComponent, pItem->m_pComponent );
                }

                canRemoveItem = !isRootComponent || pItem->GetChildren().size() <= 1;
            }

            if ( canRemoveItem )
            {
                if ( ImGui::MenuItem( EE_ICON_DELETE" Remove" ) )
                {
                    DestroyComponent( pItem->m_pComponent );
                }
            }
        }
        //-------------------------------------------------------------------------
        // System Context Menu
        //-------------------------------------------------------------------------
        else
        {
            if ( ImGui::MenuItem( EE_ICON_DELETE" Remove" ) )
            {
                DestroySystem( pItem->m_pSystem );
            }
        }
    }

    void EntityStructureEditor::HandleSelectionChanged( TreeListView::ChangeReason reason )
    {
        m_selectedSpatialComponents.clear();
        m_selectedComponents.clear();
        m_selectedSystems.clear();

        //-------------------------------------------------------------------------

        auto const& selection = GetSelection();
        IReflectedType* pLastSelectedEditableType = nullptr;
        for( auto pSelectedItem : selection )
        {
            auto pItem = static_cast<StructureEditorItem*>( pSelectedItem );
            if ( pItem->IsSpatialComponent() )
            {
                m_selectedSpatialComponents.emplace_back( pItem->m_pSpatialComponent );
                pLastSelectedEditableType = pItem->m_pSpatialComponent;
            }
            else if ( pItem->IsComponent() )
            {
                m_selectedComponents.emplace_back( pItem->m_pComponent );
                pLastSelectedEditableType = pItem->m_pComponent;
            }
            else if( pItem->IsSystem() )
            {
                m_selectedSystems.emplace_back( pItem->m_pSystem );
                pLastSelectedEditableType = pItem->m_pSystem;
            }
        }

        // Notify listeners that we want to edit the selected component
        if ( reason != TreeListView::ChangeReason::TreeRebuild )
        {
            m_onRequestedTypeToEditChanged.Execute( pLastSelectedEditableType );
        }
    }

    void EntityStructureEditor::HandleDragAndDropOnItem( TreeListViewItem* pDragAndDropTargetItem ) 
    {
        if ( ImGuiPayload const* payload = ImGui::AcceptDragDropPayload( StructureEditorItem::s_dragAndDropID, ImGuiDragDropFlags_AcceptBeforeDelivery ) )
        {
            if ( payload->IsDelivery() )
            {
                auto pRawData = (uintptr_t*) payload->Data;
                auto pSourceComponentItem = (StructureEditorItem*) *pRawData;
                auto pTargetComponentItem = (StructureEditorItem*) pDragAndDropTargetItem;

                EE_ASSERT( pSourceComponentItem->IsSpatialComponent() && pTargetComponentItem->IsSpatialComponent() );

                // Same items, nothing to do
                if ( pSourceComponentItem == pTargetComponentItem )
                {
                    return;
                }

                // We cannot re-parent ourselves to one of our children
                if ( pTargetComponentItem->m_pSpatialComponent->IsSpatialChildOf( pSourceComponentItem->m_pSpatialComponent ) )
                {
                    return;
                }

                // Perform operation
                ReparentComponent( pSourceComponentItem->m_pSpatialComponent, pTargetComponentItem->m_pSpatialComponent );
            }
        }
    }

    //-------------------------------------------------------------------------
    // Entity Operations
    //-------------------------------------------------------------------------

    void EntityStructureEditor::CreateSystem( TypeSystem::TypeInfo const* pSystemTypeInfo )
    {
        EE_ASSERT( pSystemTypeInfo != nullptr );
        EE_ASSERT( m_pEditedEntity != nullptr );
        EE_ASSERT( m_pEditedEntity->IsAddedToMap() );

        auto pActiveUndoAction = EE::New<EntityUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pWorld );
        pActiveUndoAction->RecordBeginEdit( { m_pEditedEntity } );

        //-------------------------------------------------------------------------

        m_pEditedEntity->CreateSystem( pSystemTypeInfo );

        //-------------------------------------------------------------------------

        pActiveUndoAction->RecordEndEdit();
        m_pUndoStack->RegisterAction( pActiveUndoAction );
        m_shouldRefreshEditorState = true;
    }

    void EntityStructureEditor::DestroySystem( TypeSystem::TypeID systemTypeID )
    {
        EE_ASSERT( systemTypeID.IsValid() );
        EE_ASSERT( m_pEditedEntity != nullptr );

        auto pActiveUndoAction = EE::New<EntityUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pWorld );
        pActiveUndoAction->RecordBeginEdit( { m_pEditedEntity } );

        //-------------------------------------------------------------------------

        m_pEditedEntity->DestroySystem( systemTypeID );

        //-------------------------------------------------------------------------

        pActiveUndoAction->RecordEndEdit();
        m_pUndoStack->RegisterAction( pActiveUndoAction );
        m_shouldRefreshEditorState = true;

        ClearSelection();
    }

    void EntityStructureEditor::DestroySystem( EntitySystem* pSystem )
    {
        EE_ASSERT( pSystem != nullptr );
        EE_ASSERT( m_pEditedEntity != nullptr );
        EE_ASSERT( VectorContains( m_pEditedEntity->GetSystems(), pSystem ) );

        auto pActiveUndoAction = EE::New<EntityUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pWorld );
        pActiveUndoAction->RecordBeginEdit( { m_pEditedEntity } );

        //-------------------------------------------------------------------------

        m_pEditedEntity->DestroySystem( pSystem->GetTypeID() );

        //-------------------------------------------------------------------------

        pActiveUndoAction->RecordEndEdit();
        m_pUndoStack->RegisterAction( pActiveUndoAction );
        m_shouldRefreshEditorState = true;

        ClearSelection();
    }

    void EntityStructureEditor::CreateComponent( TypeSystem::TypeInfo const* pComponentTypeInfo, ComponentID const& parentSpatialComponentID )
    {
        EE_ASSERT( pComponentTypeInfo != nullptr );
        EE_ASSERT( m_pEditedEntity != nullptr );

        auto pActiveUndoAction = EE::New<EntityUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pWorld );
        pActiveUndoAction->RecordBeginEdit( { m_pEditedEntity } );

        //-------------------------------------------------------------------------

        m_pEditedEntity->CreateComponent( pComponentTypeInfo, parentSpatialComponentID );

        //-------------------------------------------------------------------------

        pActiveUndoAction->RecordEndEdit();
        m_pUndoStack->RegisterAction( pActiveUndoAction );
        m_shouldRefreshEditorState = true;
    }

    void EntityStructureEditor::DestroyComponent( EntityComponent* pComponent )
    {
        EE_ASSERT( pComponent != nullptr );
        EE_ASSERT( m_pEditedEntity != nullptr );

        auto pActiveUndoAction = EE::New<EntityUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pWorld );
        pActiveUndoAction->RecordBeginEdit( { m_pEditedEntity } );

        //-------------------------------------------------------------------------

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

        //-------------------------------------------------------------------------

        pActiveUndoAction->RecordEndEdit();
        m_pUndoStack->RegisterAction( pActiveUndoAction );
        m_shouldRefreshEditorState = true;

        ClearSelection();
    }

    void EntityStructureEditor::RenameComponent( EntityComponent* pComponent, StringID newNameID )
    {
        EE_ASSERT( pComponent != nullptr );
        EE_ASSERT( m_pEditedEntity != nullptr );

        //-------------------------------------------------------------------------

        auto pActiveUndoAction = EE::New<EntityUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pWorld );
        pActiveUndoAction->RecordBeginEdit( { m_pEditedEntity } );

        //-------------------------------------------------------------------------

        m_pEditedEntity->RenameComponent( pComponent, newNameID );

        //-------------------------------------------------------------------------

        pActiveUndoAction->RecordEndEdit();
        m_pUndoStack->RegisterAction( pActiveUndoAction );
        m_shouldRefreshEditorState = true;
    }

    void EntityStructureEditor::ReparentComponent( SpatialEntityComponent* pComponent, SpatialEntityComponent* pNewParentComponent )
    {
        EE_ASSERT( m_pEditedEntity != nullptr );
        EE_ASSERT( pComponent != nullptr && pNewParentComponent != nullptr );
        EE_ASSERT( !pComponent->IsRootComponent() );

        // Nothing to do, the new parent is the current parent
        if ( pComponent->HasSpatialParent() && pComponent->GetSpatialParentID() == pNewParentComponent->GetID() )
        {
            return;
        }

        // Create undo action
        auto pActiveUndoAction = EE::New<EntityUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pWorld );
        pActiveUndoAction->RecordBeginEdit( { m_pEditedEntity } );
        m_pWorld->BeginComponentEdit( m_pEditedEntity );

        //-------------------------------------------------------------------------

        // Remove the component from its old parent
        pComponent->m_pSpatialParent->m_spatialChildren.erase_first( pComponent );
        pComponent->m_pSpatialParent = nullptr;

        // Add the component to its new parent
        pNewParentComponent->m_spatialChildren.emplace_back( pComponent );
        pComponent->m_pSpatialParent = pNewParentComponent;

        //-------------------------------------------------------------------------

        m_pWorld->EndComponentEdit( m_pEditedEntity );
        pActiveUndoAction->RecordEndEdit();
        m_pUndoStack->RegisterAction( pActiveUndoAction );
        m_shouldRefreshEditorState = true;
    }

    void EntityStructureEditor::MakeRootComponent( SpatialEntityComponent* pComponent )
    {
        EE_ASSERT( m_pEditedEntity != nullptr );
        EE_ASSERT( pComponent != nullptr );

        // This component is already the root
        if ( m_pEditedEntity->GetRootSpatialComponentID() == pComponent->GetID() )
        {
            return;
        }

        // Create undo action
        auto pActiveUndoAction = EE::New<EntityUndoableAction>( *m_pToolsContext->m_pTypeRegistry, m_pWorld );
        pActiveUndoAction->RecordBeginEdit( { m_pEditedEntity } );
        m_pWorld->BeginComponentEdit( m_pEditedEntity );

        //-------------------------------------------------------------------------

        // Break any cross entity links
        bool recreateSpatialAttachment = m_pEditedEntity->m_isSpatialAttachmentCreated;
        if ( recreateSpatialAttachment )
        {
            m_pEditedEntity->DestroySpatialAttachment( Entity::SpatialAttachmentRule::KeepLocalTranform );
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

        //-------------------------------------------------------------------------

        m_pWorld->EndComponentEdit( m_pEditedEntity );
        pActiveUndoAction->RecordEndEdit();
        m_pUndoStack->RegisterAction( pActiveUndoAction );
        m_shouldRefreshEditorState = true;
    }
}