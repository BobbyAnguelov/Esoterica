#pragma once

#include "EngineTools/Entity/EntityEditor/EntityEditor_Context.h"
#include "EngineTools/Entity/SharedWidgets/EntityWorldOutliner.h"
#include "EngineTools/Entity/SharedWidgets/EntityEditor.h"

#include "EngineTools/Core/Workspace.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Engine/ToolsUI/Gizmo.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EE_ENGINETOOLS_API EntityWorldEditorWorkspace : public Workspace
    {
    public:

        explicit EntityWorldEditorWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID );
        EntityWorldEditorWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, String const& displayName );

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;

    protected:

        virtual bool HasViewportWindow() const override { return true; }
        virtual bool HasViewportToolbar() const override { return true; }
        virtual void InitializeDockingLayout( ImGuiID dockspaceID ) const override;
        virtual void Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused ) override;
        virtual void DrawViewportToolbarItems( UpdateContext const& context, Render::Viewport const* pViewport ) override;
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport ) override;
        virtual bool IsDirty() const override{ return false; } // TODO
        virtual bool AlwaysAllowSaving() const override { return true; }
        virtual void OnMousePick( Render::PickingID pickingID ) override;
        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) override;
        virtual void DropResourceInViewport( ResourceID const& resourceID, Vector const& worldPosition ) override;

    protected:



        EntityEditorContext                             m_context;
        EntityWorldOutliner                             m_worldOutliner;
        EntityEditor                                    m_entityEditor;

        TVector<TypeSystem::TypeInfo const*>            m_volumeTypes;
        TVector<TypeSystem::TypeInfo const*>            m_visualizedVolumeTypes;

        // Transform
        Transform                                       m_gizmoTransform;
    };
}