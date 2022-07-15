#include "EntityEditor_Inspector.h"
#include "EntityEditor_Context.h"
#include "Engine/UpdateContext.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntitySystem.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/Imgui/ImguiStyle.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    using namespace TypeSystem;

    //-------------------------------------------------------------------------

    class EntityInternalItem final : public TreeListViewItem
    {

    public:

        EntityInternalItem( char const* pLabel )
            : TreeListViewItem()
            , m_nameID( pLabel )
        {}

        EntityInternalItem( IRegisteredType* pTypeInstance )
            : TreeListViewItem()
            , m_pInstance( pTypeInstance )
        {}

        inline IRegisteredType* GetTypeInstance() { return m_pInstance; }

        virtual StringID GetNameID() const override
        {
            if ( IsCategoryLabel() )
            {
                return m_nameID;
            }
            else if ( IsEntity() )
            {
                return Cast<Entity>( m_pInstance )->GetName();
            }
            else if ( IsComponent() )
            {
                return Cast<EntityComponent>( m_pInstance )->GetName();
            }
            else
            {
                return Cast<EntitySystem>( m_pInstance )->GetTypeID().ToStringID();
            }
        }

        virtual uint64_t GetUniqueID() const override
        {
            if ( IsCategoryLabel() )
            {
                return m_nameID.GetID();
            }
            else if ( IsEntity() )
            {
                return Cast<Entity>( m_pInstance )->GetID().ToUint64();
            }
            else if ( IsComponent() )
            {
                return Cast<EntityComponent>( m_pInstance )->GetID().ToUint64();
            }
            else
            {
                return Cast<EntitySystem>( m_pInstance )->GetTypeID().ToStringID().GetID();
            }
        }

        virtual bool HasContextMenu() const override { return !IsCategoryLabel(); }

        inline Entity* GetAsEntity() const { return TryCast<Entity>( m_pInstance ); }
        inline EntityComponent* GetAsEntityComponent() const { return TryCast<EntityComponent>( m_pInstance ); }
        inline SpatialEntityComponent* GetAsSpatialEntityComponent() const { return TryCast<SpatialEntityComponent>( m_pInstance ); }
        inline EntitySystem* GetAsEntitySystem() const { return TryCast<EntitySystem>( m_pInstance ); }

        inline bool IsCategoryLabel() const { return m_pInstance == nullptr; }
        inline bool IsEntity() const { return m_pInstance != nullptr && IsOfType<Entity>( m_pInstance ); }
        inline bool IsSystem() const { return m_pInstance != nullptr && IsOfType<EntitySystem>( m_pInstance ); }
        inline bool IsComponent() const { return m_pInstance != nullptr && IsOfType<EntityComponent>( m_pInstance ); }
        inline bool IsSpatialComponent() const { return m_pInstance != nullptr && IsOfType<SpatialEntityComponent>( m_pInstance ); }

    private:

        IRegisteredType*    m_pInstance = nullptr;
        StringID            m_nameID;
    };

    //-------------------------------------------------------------------------

    EntityEditorInspector::EntityEditorInspector( EntityEditorContext& ctx )
        : TreeListView()
        , m_context( ctx )
        , m_allSystemTypes( ctx.GetTypeRegistry().GetAllDerivedTypes( EntitySystem::GetStaticTypeID(), false, false, true ) )
        , m_allComponentTypes( ctx.GetTypeRegistry().GetAllDerivedTypes( EntityComponent::GetStaticTypeID(), false, false, true ) )
    {
        Memory::MemsetZero( m_filterBuffer, 256 * sizeof( char ) );
        m_expandItemsOnlyViaArrow = true;

        //-------------------------------------------------------------------------

        auto OnEntityStateChanged = [this] ( Entity* pEntityChanged )
        {
            if ( pEntityChanged == m_pEntity )
            {
                RebuildTree();
            }
        };

        m_entityStateChangedBindingID = Entity::OnEntityUpdated().Bind( OnEntityStateChanged );
    }

    EntityEditorInspector::~EntityEditorInspector()
    {
        Entity::OnEntityUpdated().Unbind( m_entityStateChangedBindingID );
    }

    void EntityEditorInspector::Draw( UpdateContext const& context )
    {
        if ( m_context.GetNumSelectedEntities() != 1 )
        {
            m_pEntity = nullptr;
            RebuildTree( false );
        }
        else
        {
            auto pSelectedEntity = m_context.GetSelectedEntities()[0];
            if ( m_pEntity != pSelectedEntity )
            {
                m_pEntity = pSelectedEntity;
                RebuildTree( false );
            }
        }

        //-------------------------------------------------------------------------

        if ( m_pEntity != nullptr )
        {
            // Entity Details
            //-------------------------------------------------------------------------

            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::MediumBold );
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "%s", m_pEntity->GetName().c_str() );
            }

            ImGui::SameLine( ImGui::GetContentRegionAvail().x + ImGui::GetStyle().ItemSpacing.x - 82, 0 );
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::SmallBold );
                if ( ImGuiX::ColoredButton( Colors::Green, Colors::White, EE_ICON_PLUS"ADD", ImVec2( 50, 26 ) ) )
                {
                    ImGui::OpenPopup( "Choose Add Filter" );
                }

                if ( ImGui::BeginPopup( "Choose Add Filter" ) )
                {
                    if ( ImGui::MenuItem( "Component" ) )
                    {
                        m_requestedCommand = Command::AddComponent;
                    }

                    if ( ImGui::MenuItem( "System" ) )
                    {
                        m_requestedCommand = Command::AddSystem;
                    }

                    ImGui::EndPopup();
                }

                ImGuiX::ItemTooltip( "Add Component/System" );
            }

            ImGui::SameLine( ImGui::GetContentRegionAvail().x + ImGui::GetStyle().ItemSpacing.x - 28, 0 );
            if ( ImGui::Button( EE_ICON_PLAYLIST_EDIT, ImVec2( 26, 26 ) ) )
            {
                m_context.SelectEntity( m_pEntity );
                ClearSelection();
            }
            ImGuiX::ItemTooltip( "Edit Entity Details" );

            // Entity Internals
            //-------------------------------------------------------------------------

            TreeListView::Draw();

            switch ( m_requestedCommand )
            {
                case Command::AddComponent:
                {
                    RequestAddComponent();
                }
                break;

                case Command::AddSystem:
                {
                    RequestAddSystem();
                }
                break;

                case Command::Delete:
                {
                    // Get selected tree item and clear tree selection
                    auto pItem = static_cast<EntityInternalItem*>( GetSelection()[0] );
                    ClearSelection();
                    ClearActiveItem();

                    //-------------------------------------------------------------------------

                    if ( pItem->IsSystem() )
                    {
                        // Remove from the tree
                        DestroyItem( pItem->GetUniqueID() );

                        // Destroy system
                        m_context.DestroySystem( m_pEntity, pItem->GetAsEntitySystem() );
                    }
                    else if ( pItem->IsComponent() )
                    {
                        // Remove from the tree
                        auto pComponent = Cast<EntityComponent>( pItem->GetTypeInstance() );
                        DestroyItem( pItem->GetUniqueID() );

                        // Destroy component
                        m_context.DestroyComponent( m_pEntity, pComponent );
                    }
                }
                break;
            }

            m_requestedCommand = Command::None;

            // Add Component/System Dialog
            //-------------------------------------------------------------------------

            if ( DrawAddComponentOrSystemDialog() )
            {
                if ( m_addMode == AddItemMode::Components )
                {
                    ComponentID parentComponentID = ( m_pParentSpatialComponent != nullptr ) ? m_pParentSpatialComponent->GetID() : ComponentID();
                    m_context.CreateComponent( m_pEntity, m_pSelectedType, parentComponentID );
                }
                else if ( m_addMode == AddItemMode::Systems )
                {
                    m_context.CreateSystem( m_pEntity, m_pSelectedType );
                }

                m_addMode = AddItemMode::None;
                m_pParentSpatialComponent = nullptr;
            }
        }
    }

    //-------------------------------------------------------------------------

    static void FillTree( EntityInternalItem* pParentItem, TInlineVector<SpatialEntityComponent*, 10> const& spatialComponents )
    {
        EE_ASSERT( pParentItem != nullptr );

        for ( auto i = 0u; i < spatialComponents.size(); i++ )
        {
            if ( spatialComponents[i]->GetSpatialParentID() == pParentItem->GetAsSpatialEntityComponent()->GetID() )
            {
                auto pNewParentSpatialComponent = pParentItem->CreateChild<EntityInternalItem>( spatialComponents[i] );
                pNewParentSpatialComponent->SetExpanded( true );
                FillTree( pNewParentSpatialComponent, spatialComponents );
            }
        }
    }

    void EntityEditorInspector::RebuildTreeInternal()
    {
        if ( m_pEntity == nullptr )
        {
            return;
        }

        if ( m_pEntity->HasStateChangeActionsPending() )
        {
            return;
        }

        // Systems
        //-------------------------------------------------------------------------

        if ( !m_pEntity->GetSystems().empty() )
        {
            auto pSystemsRoot = m_rootItem.CreateChild<EntityInternalItem>( "Systems" );
            pSystemsRoot->SetExpanded( true );
            for ( auto pSystem : m_pEntity->GetSystems() )
            {
                pSystemsRoot->CreateChild<EntityInternalItem>( pSystem );
            }
        }

        // Components
        //-------------------------------------------------------------------------

        TInlineVector<EntityComponent*, 10> components;
        TInlineVector<SpatialEntityComponent*, 10> spatialComponents;

        for ( auto pComponent : m_pEntity->GetComponents() )
        {
            if ( auto pSpatialComponent = TryCast<SpatialEntityComponent>( pComponent ) )
            {
                spatialComponents.emplace_back( pSpatialComponent );
            }
            else
            {
                components.emplace_back( pComponent );
            }
        }

        if ( !components.empty() )
        {
            auto pComponentsRoot = m_rootItem.CreateChild<EntityInternalItem>( "Components" );
            pComponentsRoot->SetExpanded( true );
            for ( auto pComponent : components )
            {
                pComponentsRoot->CreateChild<EntityInternalItem>( pComponent );
            }
        }

        if ( !spatialComponents.empty() )
        {
            auto pComponentsRoot = m_rootItem.CreateChild<EntityInternalItem>( "Spatial Components" );
            pComponentsRoot->SetExpanded( true );

            auto pParentSpatialComponent = pComponentsRoot->CreateChild<EntityInternalItem>( m_pEntity->GetRootSpatialComponent() );
            pParentSpatialComponent->SetExpanded( true );
            spatialComponents.erase_first_unsorted( m_pEntity->GetRootSpatialComponent() );

            FillTree( pParentSpatialComponent, spatialComponents );
        }
    }

    void EntityEditorInspector::DrawItemContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus )
    {
        auto pItem = static_cast<EntityInternalItem*>( selectedItemsWithContextMenus[0] );
        EE_ASSERT( !pItem->IsCategoryLabel() );

        //-------------------------------------------------------------------------

        if ( pItem->IsEntity() )
        {
            if ( ImGui::MenuItem( "Add System" ) )
            {
                m_requestedCommand = Command::AddSystem;
            }

            if ( ImGui::MenuItem( "Add Component" ) )
            {
                m_requestedCommand = Command::AddComponent;
            }
        }

        //-------------------------------------------------------------------------

        if ( pItem->IsSpatialComponent() )
        {
            if ( ImGui::MenuItem( "Add Child Component" ) )
            {
                m_pParentSpatialComponent = Cast<SpatialEntityComponent>( pItem->GetTypeInstance() );
                m_requestedCommand = Command::AddComponent;
            }
        }

        //-------------------------------------------------------------------------

        if ( pItem->IsSystem() || pItem->IsComponent() )
        {
            if ( ImGui::MenuItem( "Delete" ) )
            {
                m_requestedCommand = Command::Delete;
            }
        }
    }

    void EntityEditorInspector::OnSelectionChangedInternal()
    {
        auto pItem = static_cast<EntityInternalItem*>( GetSelection()[0] );
        if ( pItem == nullptr || pItem->IsEntity() )
        {
            m_context.SelectEntity( pItem->GetAsEntity() );
        }
        else if ( pItem->IsComponent() )
        {
            m_context.SelectComponent( pItem->GetAsEntityComponent() );
        }
    }

    //-------------------------------------------------------------------------

    void EntityEditorInspector::RequestAddSystem()
    {
        EE_ASSERT( m_pEntity != nullptr );

        TInlineVector<TypeSystem::TypeID, 10> restrictions;
        for ( auto pSystem : m_pEntity->GetSystems() )
        {
            restrictions.emplace_back( pSystem->GetTypeID() );
        }

        //-------------------------------------------------------------------------

        m_systemOptions.clear();
        for ( auto pSystemTypeInfo : m_allSystemTypes )
        {
            bool isValidOption = true;
            for ( auto pExistingSystem : m_pEntity->GetSystems() )
            {
                if ( m_context.GetTypeRegistry().AreTypesInTheSameHierarchy( pExistingSystem->GetTypeInfo(), pSystemTypeInfo ) )
                {
                    isValidOption = false;
                    break;
                }
            }

            if ( isValidOption )
            {
                m_systemOptions.emplace_back( pSystemTypeInfo );
            }
        }

        m_addMode = AddItemMode::Systems;
        m_initializeFocus = true;
        ImGui::OpenPopup( s_addSystemDialogTitle );
    }

    void EntityEditorInspector::RequestAddComponent()
    {
        EE_ASSERT( m_pEntity != nullptr );

        TInlineVector<TypeSystem::TypeID, 10> restrictions;
        for ( auto pComponent : m_pEntity->GetComponents() )
        {
            if ( pComponent->IsSingletonComponent() )
            {
                restrictions.emplace_back( pComponent->GetTypeID() );
            }
        }

        //-------------------------------------------------------------------------

        m_componentOptions.clear();
        for ( auto pComponentTypeInfo : m_allComponentTypes )
        {
            bool isValidOption = true;
            for ( auto pExistingComponent :m_pEntity->GetComponents() )
            {
                if ( pExistingComponent->IsSingletonComponent() )
                {
                    if ( m_context.GetTypeRegistry().AreTypesInTheSameHierarchy( pExistingComponent->GetTypeInfo(), pComponentTypeInfo ) )
                    {
                        isValidOption = false;
                        break;
                    }
                }
            }

            if ( isValidOption )
            {
                m_componentOptions.emplace_back( pComponentTypeInfo );
            }
        }

        m_addMode = AddItemMode::Components;
        m_initializeFocus = true;
        ImGui::OpenPopup( s_addComponentDialogTitle );
    }

    bool EntityEditorInspector::DrawAddComponentOrSystemDialog()
    {
        bool isOpen = true;
        bool selectionMade = false;

        ImGui::SetNextWindowSize( ImVec2( 1000, 400 ), ImGuiCond_FirstUseEver );
        ImGui::SetNextWindowSizeConstraints( ImVec2( 400, 400 ), ImVec2( FLT_MAX, FLT_MAX ) );
        if ( ImGui::BeginPopupModal( ( m_addMode == AddItemMode::Components ) ? s_addComponentDialogTitle : s_addSystemDialogTitle, &isOpen ) )
        {
            ImVec2 const contentRegionAvailable = ImGui::GetContentRegionAvail();

            // Draw Filter
            //-------------------------------------------------------------------------

            bool filterUpdated = false;

            ImGui::SetNextItemWidth( contentRegionAvailable.x - ImGui::GetStyle().WindowPadding.x - 26 );
            InlineString filterCopy( m_filterBuffer );

            if ( m_initializeFocus )
            {
                ImGui::SetKeyboardFocusHere();
                m_initializeFocus = false;
                filterUpdated = true;
            }

            if ( ImGui::InputText( "##Filter", filterCopy.data(), 256 ) )
            {
                if ( strcmp( filterCopy.data(), m_filterBuffer ) != 0 )
                {
                    strcpy_s( m_filterBuffer, 256, filterCopy.data() );

                    // Convert buffer to lower case
                    int32_t i = 0;
                    while ( i < 256 && m_filterBuffer[i] != 0 )
                    {
                        m_filterBuffer[i] = eastl::CharToLower( m_filterBuffer[i] );
                        i++;
                    }

                    filterUpdated = true;
                }
            }

            ImGui::SameLine();
            if ( ImGui::Button( EE_ICON_CLOSE_CIRCLE "##Clear Filter", ImVec2( 26, 0 ) ) )
            {
                m_filterBuffer[0] = 0;
                filterUpdated = true;
            }

            // Update filter options
            //-------------------------------------------------------------------------

            if ( filterUpdated )
            {
                auto& allOptions = ( m_addMode == AddItemMode::Components ) ? m_componentOptions : m_systemOptions;

                if ( m_filterBuffer[0] == 0 )
                {
                    m_filteredOptions = allOptions;
                }
                else
                {
                    m_filteredOptions.clear();
                    for ( auto const& pTypeInfo : allOptions )
                    {
                        String lowercasePath( pTypeInfo->GetTypeName() );
                        lowercasePath.make_lower();

                        bool passesFilter = true;
                        char* token = strtok( m_filterBuffer, " " );
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
            }

            // Draw results
            //-------------------------------------------------------------------------

            float const tableHeight = contentRegionAvailable.y - ImGui::GetFrameHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.y;
            if ( ImGui::BeginTable( "Options List", 1, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2( contentRegionAvailable.x, tableHeight ) ) )
            {
                ImGui::TableSetupColumn( "Type", ImGuiTableColumnFlags_WidthStretch, 1.0f );

                //-------------------------------------------------------------------------

                ImGui::TableHeadersRow();

                ImGuiListClipper clipper;
                clipper.Begin( (int32_t) m_filteredOptions.size() );
                while ( clipper.Step() )
                {
                    for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                    {
                        ImGui::TableNextRow();

                        ImGui::TableNextColumn();
                        bool isSelected = ( m_pSelectedType == m_filteredOptions[i] );
                        if ( ImGui::Selectable( m_filteredOptions[i]->GetFriendlyTypeName(), &isSelected, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns ) )
                        {
                            m_pSelectedType = m_filteredOptions[i];

                            if ( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
                            {
                                selectionMade = true;
                            }
                        }
                    }
                }

                ImGui::EndTable();
            }

            //-------------------------------------------------------------------------

            if ( selectionMade )
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        return selectionMade;
    }
}