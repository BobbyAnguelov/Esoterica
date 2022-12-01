#pragma once

#include "EngineTools/Entity/EntityEditor/EntityWorldOutliner.h"
#include "EngineTools/Entity/EntityEditor/EntityStructureEditor.h"
#include "EngineTools/Entity/EntityEditor/EntityWorldPropertyGrid.h"
#include "EngineTools/Core/Workspace.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EntityUndoableAction;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API EntityEditorWorkspace : public Workspace
    {
    public:

        explicit EntityEditorWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID );
        EntityEditorWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, String const& displayName );

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;

    protected:

        virtual bool ShouldLoadDefaultEditorMap() const override { return false; }
        virtual bool HasViewportWindow() const override { return true; }
        virtual bool HasViewportToolbar() const override { return true; }
        virtual void InitializeDockingLayout( ImGuiID dockspaceID ) const override;
        virtual void Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused ) override;
        virtual void DrawViewportToolbarItems( UpdateContext const& context, Render::Viewport const* pViewport ) override;
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport ) override;
        virtual bool AlwaysAllowSaving() const override { return true; }
        virtual void OnMousePick( Render::PickingID pickingID ) override;
        virtual void DropResourceInViewport( ResourceID const& resourceID, Vector const& worldPosition ) override;

        virtual void PreUndoRedo( UndoStack::Operation operation ) override;
        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) override;

        // Events
        //-------------------------------------------------------------------------

        void OnOutlinerSelectionChanged( TreeListView::ChangeReason reason );
        void OnStructureEditorSelectionChanged( IRegisteredType* pTypeToEdit );
        void OnActionPerformed();
        void UpdateSelectionSpatialInfo();

        // Transform
        //-------------------------------------------------------------------------

        void BeginTransformManipulation( Transform const& newTransform, bool duplicateSelection = false );
        void ApplyTransformManipulation( Transform const& newTransform );
        void EndTransformManipulation( Transform const& newTransform );

    protected:

        // Outliner
        EntityWorldOutliner                             m_outliner;
        EventBindingID                                  m_outlinerSelectionUpdateEventBindingID;
        EventBindingID                                  m_actionPerformEventBindingID;

        // Structure editor
        EntityStructureEditor                           m_entityStructureEditor;
        EventBindingID                                  m_structureEditorSelectionUpdateEventBindingID;

        // Property Grid
        EntityWorldPropertyGrid                         m_propertyGrid;

        // Visualization
        TVector<TypeSystem::TypeInfo const*>            m_volumeTypes;
        TVector<TypeSystem::TypeInfo const*>            m_visualizedVolumeTypes;

        // Transform manipulation
        OBB                                             m_selectionBounds;
        Transform                                       m_selectionTransform;
        TVector<Transform>                              m_selectionOffsetTransforms;
        bool                                            m_hasSpatialSelection = false;
    };
}