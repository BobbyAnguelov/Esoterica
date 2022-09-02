#pragma once
#include "EngineTools/_Module/API.h"
#include "EngineTools/Core/Widgets/TreeListView.h"
#include "EngineTools/Core/PropertyGrid/PropertyGrid.h"
#include "System/TypeSystem/TypeID.h"

//-------------------------------------------------------------------------

struct ImGuiWindowClass;

namespace EE
{
    class Entity;
    class SpatialEntityComponent;
    class UpdateContext;

    namespace TypeSystem
    { 
        class TypeRegistry;
        class TypeInfo;
    }
}

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EntityEditorContext;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API EntityEditor final : public TreeListView
    {
        enum class Command
        {
            None,
            AddSystem,
            AddComponent,
            Delete
        };

        enum class AddItemMode
        {
            None,
            Systems,
            Components
        };

        constexpr static const char* const s_addSystemDialogTitle = "Add System##ASC";
        constexpr static const char* const s_addComponentDialogTitle = "Add Component##ASC";

    public:

        EntityEditor( ToolsContext const* pToolsContext, EntityEditorContext& ctx );
        ~EntityEditor();

        void Initialize( UpdateContext const& context, uint32_t widgetUniqueID );
        void Shutdown( UpdateContext const& context );

        inline char const* GetPropertyGridWindowName() const { return m_propertyGridWindowName.c_str(); }
        inline char const* GetEntityStructureWindowName() const { return m_entityStructureWindowName.c_str(); }

        void SetEntityToEdit( Entity* pEntity );

        void UpdateAndDraw( UpdateContext const& context, ImGuiWindowClass* pWindowClass );
        void Refresh() { RebuildTree( false ); }

    private:

        bool DrawDialogs();

        virtual void RebuildTreeInternal() override;
        virtual void DrawItemContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus ) override;
        virtual void OnSelectionChangedInternal() override;

        void RequestAddSystem();
        void RequestAddComponent();

        // Called whenever the internal structure of an entity changes to update all system/component options

        // Entity Structure
        //-------------------------------------------------------------------------

        void DrawEntityStructureWindow( UpdateContext const& context, ImGuiWindowClass* pWindowClass );
        void UpdateAddableSystemAndComponentOptions();


        // Property Grid
        //-------------------------------------------------------------------------

        void DrawPropertyGridWindow( UpdateContext const& context, ImGuiWindowClass* pWindowClass );
        void PreEditProperty( PropertyEditInfo const& eventInfo );
        void PostEditProperty( PropertyEditInfo const& eventInfo );

    private:

        EntityEditorContext&                            m_context;
        EventBindingID                                  m_entityStateChangedBindingID;
        Command                                         m_requestedCommand = Command::None;
        String                                          m_entityStructureWindowName;
        String                                          m_propertyGridWindowName;

        TVector<TypeSystem::TypeInfo const*> const      m_allSystemTypes;
        TVector<TypeSystem::TypeInfo const*> const      m_allComponentTypes;

        // Edited entity
        Entity*                                         m_pEditedEntity = nullptr;
        bool                                            m_shouldRefreshEditorState = false;

        // Property grid
        EventBindingID                                  m_preEditPropertyBindingID;
        EventBindingID                                  m_postEditPropertyBindingID;
        PropertyGrid                                    m_propertyGrid;

        // Cached filtered lists of selectable options
        TVector<TypeSystem::TypeInfo const*>            m_systemOptions;
        TVector<TypeSystem::TypeInfo const*>            m_componentOptions;
        TVector<TypeSystem::TypeInfo const*>            m_filteredOptions;
        char                                            m_filterBuffer[256];

        AddItemMode                                     m_addMode = AddItemMode::None;
        SpatialEntityComponent*                         m_pParentSpatialComponent = nullptr;
        TypeSystem::TypeInfo const*                     m_pSelectedType = nullptr;
        TInlineVector<TypeSystem::TypeID, 10>           m_excludedTypes;
        bool                                            m_initializeFocus = false;
    };
}