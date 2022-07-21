#pragma once
#include "EngineTools/Core/UndoStack.h"
#include "EngineTools/Resource/ResourceDatabase.h"
#include "EngineTools/Core/ToolsContext.h"
#include "System/Imgui/ImguiX.h"
#include "System/Render/RenderTarget.h"
#include "System/Resource/ResourceRequesterID.h"
#include "System/Resource/ResourcePtr.h"
#include "System/FileSystem/FileSystemPath.h"
#include "System/Drawing/DebugDrawing.h"
#include "System/Types/Function.h"

//-------------------------------------------------------------------------

namespace EE
{
    class UpdateContext;
    class Entity;
    class EntityWorld;
    class EntityWorldUpdateContext;
    namespace Render { class Viewport; }
}

//-------------------------------------------------------------------------
// Editor Workspace
//-------------------------------------------------------------------------
// This is a base class to create a workspace within the editor
// Provides a lot of utility functions for loading resource/creating entities that must be used!

namespace EE
{
    class EE_ENGINETOOLS_API EditorWorkspace
    {
    public:

        struct ViewportInfo
        {
            ImTextureID                                     m_pViewportRenderTargetTexture = nullptr;
            TFunction<Render::PickingID( Int2 const& )>     m_retrievePickingID;
        };

    public:

        EditorWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, String const& displayName = "Workspace" );
        virtual ~EditorWorkspace();

        // Get the display name for this workspace (shown on tab, dialogs, etc...)
        virtual char const* GetDisplayName() const { return m_displayName.c_str(); }

        // Get the main workspace window ID - Needs to be unique per workspace instance!
        inline char const* GetWorkspaceWindowID() const { EE_ASSERT( !m_workspaceWindowID.empty() ); return m_workspaceWindowID.c_str(); }

        // Get the viewport window name/ID - Needs to be unique per workspace instance!
        inline char const* GetViewportWindowID() const { EE_ASSERT( !m_viewportWindowID.empty() ); return m_viewportWindowID.c_str(); }

        // Get the main workspace window ID - Needs to be unique per workspace instance!
        inline char const* GetDockspaceID() const { EE_ASSERT( !m_dockspaceID.empty() ); return m_dockspaceID.c_str(); }

        // Get a unique ID for this workspace
        virtual uint32_t GetID() const = 0;

        // Should this workspace display a viewport?
        virtual bool HasViewportWindow() const { return true; }

        // Does this workspace have a toolbar?
        virtual bool HasWorkspaceToolbar() const { return true; }

        // Does this workspace's viewport have the default items (save/undo/redo)?
        virtual bool HasWorkspaceToolbarDefaultItems() const { return true; }

        // Does this workspace's viewport have a toolbar?
        virtual bool HasViewportToolbar() const { return false; }

        // Should we draw the time control widget in the viewport toolbar
        virtual bool HasViewportToolbarTimeControls() const { return false; }

        // Does this workspace's viewport have a orientation guide drawn?
        virtual bool HasViewportOrientationGuide() const { return true; }

        // Get the world associated with this workspace
        inline EntityWorld* GetWorld() const { return m_pWorld; }

        // Lifetime/Update Functions
        //-------------------------------------------------------------------------

        // Initialize the workspace: initialize window IDs, create preview entities, etc... - Base implementation must be called!
        virtual void Initialize( UpdateContext const& context );
        virtual void Shutdown( UpdateContext const& context ) {}

        // Called just before the world is updated per update stage
        virtual void UpdateWorld( EntityWorldUpdateContext const& updateContext ) {}

        // Frame update and draw any tool windows needed for the workspace
        virtual void UpdateWorkspace( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused ) = 0;

        // Called by the editor before the main update, this handles a lot of the shared functionality (undo/redo/etc...)
        void SharedUpdateWorkspace( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused );

        // Drawing Functions
        //-------------------------------------------------------------------------

        // Set up initial docking layout
        virtual void InitializeDockingLayout( ImGuiID dockspaceID ) const = 0;

        // Draw any toolbar buttons that this workspace needs
        virtual void DrawWorkspaceToolbarItems( UpdateContext const& context ) {}

        // Draws the workspace toolbar menu
        void DrawWorkspaceToolbar( UpdateContext const& context );

        // Called within the context of a large overlay window allowing you to draw helpers and widgets over a viewport
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport ) {}

        // Draw the viewport toolbar
        virtual void DrawViewportToolbarItems( UpdateContext const& context, Render::Viewport const* pViewport ) {}

        // Draw the viewport for this workspace
        bool DrawViewport( UpdateContext const& context, ViewportInfo const& viewportInfo, ImGuiWindowClass* pWindowClass );

        // Preview World Functions
        //-------------------------------------------------------------------------

        void SetViewportCameraSpeed( float cameraSpeed );

        void SetViewportCameraPosition( Transform const& cameraTransform );

        void SetWorldPaused( bool isPaused );

        void SetWorldTimeScale( float newTimeScale );

        void ResetWorldTimeScale();

        // Undo/Redo
        //-------------------------------------------------------------------------

        // Called whenever we execute an undo or redo action
        virtual void PreUndoRedo( UndoStack::Operation operation ) {}
        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) {}

        inline bool CanUndo() { return m_undoStack.CanUndo(); }
        inline void Undo() { PreUndoRedo( UndoStack::Operation::Undo ); auto pAction = m_undoStack.Undo(); PostUndoRedo( UndoStack::Operation::Undo, pAction ); }
        inline bool CanRedo() { return m_undoStack.CanRedo(); }
        inline void Redo() { PreUndoRedo( UndoStack::Operation::Redo ); auto pAction = m_undoStack.Redo(); PostUndoRedo( UndoStack::Operation::Redo, pAction ); }

        // Saving and Dirty state
        //-------------------------------------------------------------------------

        // Has any modifications been made to this file?
        virtual bool IsDirty() const { return false; }

        // Should we always allow saving?
        virtual bool AlwaysAllowSaving() const { return false; }

        // Optional: Save functionality for files that support it
        virtual bool Save() { return false; }

        // Hot-reload
        //-------------------------------------------------------------------------

        inline bool IsHotReloading() const { return !m_reloadingResources.empty(); }
        virtual void BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToBeReloaded, TVector<ResourceID> const& resourcesToBeReloaded );
        virtual void EndHotReload();

    protected:

        // Called whenever we click inside a workspace viewport and get a non-zero picking ID
        virtual void OnMousePick( Render::PickingID pickingID ) {}

        // Called during the viewport drawing allowing the workspaces to handle drag and drop requests
        virtual void OnDragAndDrop( Render::Viewport* pViewport ) {}

        // Helper function for debug drawing
        Drawing::DrawContext GetDrawingContext();

        // Set the workspace tab-title
        void SetDisplayName( String const& name );

        // Draws the viewport toolbar
        void DrawViewportToolbar( UpdateContext const& context, Render::Viewport const* pViewport );

        // Begin a toolbar group
        bool BeginViewportToolbarGroup( char const* pGroupID, ImVec2 groupSize, ImVec2 const& padding = ImVec2( 4.0f, 4.0f ) );

        // End a toolbar group
        void EndViewportToolbarGroup();

        // Resource helpers
        //-------------------------------------------------------------------------

        inline FileSystem::Path const& GetRawResourceDirectoryPath() const { return m_pToolsContext->m_pResourceDatabase->GetRawResourceDirectoryPath(); }
        inline FileSystem::Path const& GetCompiledResourceDirectoryPath() const { return m_pToolsContext->m_pResourceDatabase->GetCompiledResourceDirectoryPath(); }

        inline FileSystem::Path GetFileSystemPath( ResourcePath const& resourcePath ) const
        {
            EE_ASSERT( resourcePath.IsValid() );
            return resourcePath.ToFileSystemPath( m_pToolsContext->m_pResourceDatabase->GetRawResourceDirectoryPath() );
        }

        inline FileSystem::Path GetFileSystemPath( ResourceID const& resourceID ) const
        {
            EE_ASSERT( resourceID.IsValid() );
            return resourceID.GetResourcePath().ToFileSystemPath( m_pToolsContext->m_pResourceDatabase->GetRawResourceDirectoryPath() );
        }

        inline ResourcePath GetResourcePath( FileSystem::Path const& path ) const
        {
            EE_ASSERT( path.IsValid() );
            return ResourcePath::FromFileSystemPath( m_pToolsContext->m_pResourceDatabase->GetRawResourceDirectoryPath(), path );
        }

        // Resource and Entity Management
        //-------------------------------------------------------------------------

        void LoadResource( Resource::ResourcePtr* pResourcePtr );
        void UnloadResource( Resource::ResourcePtr* pResourcePtr );

        // Add an entity to the preview world
        // Ownership is transferred to the world, you dont need to call remove/destroy if you dont explicitly need to remove an entity
        void AddEntityToWorld( Entity* pEntity );

        // Removes an entity from the preview world
        // Ownership is transferred back to calling code, so you need to delete it manually
        void RemoveEntityFromWorld( Entity* pEntity );

        // Destroys an entity from the world - pEntity will be set to nullptr
        void DestroyEntityInWorld( Entity*& pEntity );

    private:

        EditorWorkspace& operator=( EditorWorkspace const& ) = delete;
        EditorWorkspace( EditorWorkspace const& ) = delete;

    protected:

        EntityWorld*                                m_pWorld = nullptr;
        ToolsContext const*                         m_pToolsContext = nullptr;

        UndoStack                                   m_undoStack;
        String                                      m_displayName;
        String                                      m_workspaceWindowID;
        String                                      m_viewportWindowID;
        String                                      m_dockspaceID;
        bool                                        m_isViewportFocused = false;
        bool                                        m_isViewportHovered = false;

    private:

        // Time controls
        float                                       m_worldTimeScale = 1.0f;

        // Hot-reloading
        TVector<Resource::ResourcePtr*>             m_requestedResources;
        TVector<Entity*>                            m_addedEntities;
        TVector<Resource::ResourcePtr*>             m_reloadingResources;
    };
}