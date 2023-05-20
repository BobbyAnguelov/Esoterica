#pragma once
#include "Engine/Entity/EntityIDs.h"
#include "EngineTools/Core/PropertyGrid/PropertyGrid.h"

//-------------------------------------------------------------------------

struct ImGuiWindowClass;

namespace EE
{
    class ToolsContext;
    class UpdateContext;
    class UndoStack;
    class Entity;
    class EntityComponent;
    class SpatialEntityComponent;
    class EntitySystem;
    class EntityWorld;

    namespace TypeSystem
    {
        class TypeRegistry;
        class TypeInfo;
    }
}

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EntityUndoableAction;

    //-------------------------------------------------------------------------

    class EntityWorldPropertyGrid
    {
    public:

        EntityWorldPropertyGrid( ToolsContext const* pToolsContext, UndoStack* pUndoStack, EntityWorld* pWorld );
        ~EntityWorldPropertyGrid();

        void Initialize( UpdateContext const& context, uint32_t widgetUniqueID );
        void Shutdown( UpdateContext const& context );

        inline char const* GetWindowName() const { return m_windowName.c_str(); }

        // Update and draw the window - returns true if the window is focused
        bool UpdateAndDraw( UpdateContext const& context, ImGuiWindowClass* pWindowClass );

        // Request that the property grid resolves the IDs to a new type instance
        inline void RequestRefresh() { m_requestRefresh = true; }

        // Set the type instance to edit (only entities and components are supported!)
        void SetTypeInstanceToEdit( IReflectedType* pTypeInstance );

    private:

        void Clear();

        // Set the actual edited type instance based on the recorded IDs
        // Return true if the resolution of the IDs was successful
        bool RefreshEditedTypeInstance();

        void PreEditProperty( PropertyEditInfo const& eventInfo );
        void PostEditProperty( PropertyEditInfo const& eventInfo );

    private:

        ToolsContext const*                             m_pToolsContext = nullptr;
        UndoStack*                                      m_pUndoStack = nullptr;
        EntityWorld*                                    m_pWorld = nullptr;

        EntityUndoableAction*                           m_pActiveUndoAction = nullptr;

        PropertyGrid                                    m_propertyGrid;
        String                                          m_windowName;
        EventBindingID                                  m_preEditPropertyBindingID;
        EventBindingID                                  m_postEditPropertyBindingID;

        EntityID                                        m_editedEntityID;
        ComponentID                                     m_editedComponentID;

        bool                                            m_requestRefresh = false;
    };
}