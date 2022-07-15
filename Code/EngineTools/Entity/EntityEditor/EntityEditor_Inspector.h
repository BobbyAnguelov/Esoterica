#pragma once
#include "EngineTools/_Module/API.h"
#include "EngineTools/Core/Widgets/TreeListView.h"
#include "System/TypeSystem/TypeID.h"

//-------------------------------------------------------------------------

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

    class EE_ENGINETOOLS_API EntityEditorInspector final : public TreeListView
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

        EntityEditorInspector( EntityEditorContext& ctx );
        ~EntityEditorInspector();

        void Draw( UpdateContext const& context );
        void Refresh() { RebuildTree( false ); }

    private:

        virtual void RebuildTreeInternal() override;
        virtual void DrawItemContextMenu( TVector<TreeListViewItem*> const& selectedItemsWithContextMenus ) override;
        virtual void OnSelectionChangedInternal() override;

        void RequestAddSystem();
        void RequestAddComponent();
        bool DrawAddComponentOrSystemDialog();

    private:

        EntityEditorContext&                            m_context;
        EventBindingID                                  m_entityStateChangedBindingID;
        Entity*                                         m_pEntity = nullptr;
        Command                                         m_requestedCommand = Command::None;

        char                                            m_filterBuffer[256];

        TVector<TypeSystem::TypeInfo const*> const      m_allSystemTypes;
        TVector<TypeSystem::TypeInfo const*> const      m_allComponentTypes;
        TVector<TypeSystem::TypeInfo const*>            m_systemOptions;
        TVector<TypeSystem::TypeInfo const*>            m_componentOptions;
        TVector<TypeSystem::TypeInfo const*>            m_filteredOptions;

        AddItemMode                                     m_addMode = AddItemMode::None;
        SpatialEntityComponent*                         m_pParentSpatialComponent = nullptr;
        TypeSystem::TypeInfo const*                     m_pSelectedType = nullptr;
        TInlineVector<TypeSystem::TypeID, 10>           m_excludedTypes;
        bool                                            m_initializeFocus = false;
    };
}