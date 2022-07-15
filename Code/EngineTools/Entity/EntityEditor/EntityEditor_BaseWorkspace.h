#pragma once

#include "EngineTools/Entity/EntityEditor/EntityEditor_Context.h"
#include "EngineTools/Entity/EntityEditor/EntityEditor_Outliner.h"
#include "EngineTools/Entity/EntityEditor/EntityEditor_Inspector.h"
#include "EngineTools/Entity/EntityEditor/EntityEditor_PropertyGrid.h"
#include "EngineTools/Core/Workspaces/EditorWorkspace.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Engine/ToolsUI/Gizmo.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EE_ENGINETOOLS_API EntityEditorBaseWorkspace : public EditorWorkspace
    {
    public:

        EntityEditorBaseWorkspace( ToolsContext const* pToolsContext, EntityWorld * pWorld );

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;

    protected:

        virtual bool HasViewportToolbar() const override { return true; }
        virtual void InitializeDockingLayout( ImGuiID dockspaceID ) const override;
        virtual void UpdateWorkspace( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused ) override;
        virtual void DrawViewportToolbarItems( UpdateContext const& context, Render::Viewport const* pViewport ) override;
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport ) override;
        virtual bool IsDirty() const override{ return false; } // TODO
        virtual bool AlwaysAllowSaving() const override { return true; }
        virtual void OnMousePick( Render::PickingID pickingID ) override;
        virtual void OnDragAndDrop( Render::Viewport* pViewport ) override;
        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) override;

        // Called whenever we have a valid resource being dropped into the viewport, users can override to provide custom handling
        virtual void DropResourceInMap( ResourceID const& resourceID, Vector const& worldPosition );

    protected:

        String                                          m_outlinerWindowName;
        String                                          m_entityViewWindowName;
        String                                          m_propertyGridWindowName;

        EntityEditorContext                             m_context;
        EntityEditorOutliner                            m_entityOutliner;
        EntityEditorInspector                           m_entityInspector;
        EntityEditorPropertyGrid                        m_propertyGrid;

        TVector<TypeSystem::TypeInfo const*>            m_volumeTypes;
        TVector<TypeSystem::TypeInfo const*>            m_visualizedVolumeTypes;

        ImGuiX::Gizmo                                   m_gizmo;
        Transform                                       m_gizmoTransform;
    };
}