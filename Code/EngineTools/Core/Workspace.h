#pragma once
#include "UndoStack.h"
#include "ToolsContext.h"
#include "PropertyGrid/PropertyGrid.h"
#include "Helpers/GlobalRegistryBase.h"
#include "EngineTools/Resource/ResourceDatabase.h"
#include "Engine/ToolsUI/Gizmo.h"
#include "System/Imgui/ImguiX.h"
#include "System/Render/RenderTarget.h"
#include "System/Resource/ResourceRequesterID.h"
#include "System/Resource/ResourcePtr.h"
#include "System/FileSystem/FileSystemPath.h"
#include "System/Serialization/JsonSerialization.h"
#include "System/Drawing/DebugDrawing.h"
#include "System/Types/Function.h"

//-------------------------------------------------------------------------

namespace EE
{
    class UpdateContext;
    class Entity;
    class EntityWorld;
    class EntityWorldUpdateContext;
    class DebugCameraComponent;
    class ResourceDescriptorUndoableAction;
    namespace Render { class Viewport; }
    namespace Resource { struct ResourceDescriptor; }
}

//-------------------------------------------------------------------------
// Editor Workspace
//-------------------------------------------------------------------------
// This is a base class to create a workspace within the editor
// Provides a lot of utility functions for loading resource/creating entities that must be used!

namespace EE
{
    class EE_ENGINETOOLS_API Workspace
    {
        friend class ScopedDescriptorModification;
        friend ResourceDescriptorUndoableAction;

    public:

        struct ViewportInfo
        {
            ImTextureID                                     m_pViewportRenderTargetTexture = nullptr;
            TFunction<Render::PickingID( Int2 const& )>     m_retrievePickingID;
        };

    public:

        Workspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID );
        Workspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, String const& displayName );
        virtual ~Workspace();

        // Get a unique ID for this workspace
        virtual uint32_t GetID() const { return m_descriptorID.GetPathID(); }

        // Get the display name for this workspace (shown on tab, dialogs, etc...)
        virtual char const* GetDisplayName() const { return m_displayName.c_str(); }

        // Get the main workspace window ID - Needs to be unique per workspace instance!
        inline char const* GetWorkspaceWindowID() const { EE_ASSERT( !m_workspaceWindowID.empty() ); return m_workspaceWindowID.c_str(); }

        // Get the viewport window name/ID - Needs to be unique per workspace instance!
        inline char const* GetViewportWindowID() const { EE_ASSERT( !m_viewportWindowID.empty() ); return m_viewportWindowID.c_str(); }

        // Get the main workspace window ID - Needs to be unique per workspace instance!
        inline char const* GetDockspaceID() const { EE_ASSERT( !m_dockspaceID.empty() ); return m_dockspaceID.c_str(); }

        // Should this workspace display a viewport?
        virtual bool HasViewportWindow() const { return false; }

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
        virtual void Shutdown( UpdateContext const& context );

        // Called just before the world is updated per update stage
        virtual void PreUpdateWorld( EntityWorldUpdateContext const& updateContext ) {}

        // Frame update and draw any tool windows needed for the workspace
        virtual void Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused );

        // Called by the editor before the main update, this handles a lot of the shared functionality (undo/redo/etc...)
        void InternalSharedUpdate( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused );

        // Drawing Functions
        //-------------------------------------------------------------------------

        // Set up initial docking layout
        virtual void InitializeDockingLayout( ImGuiID dockspaceID ) const;

        // Draw any toolbar buttons that this workspace needs
        virtual void DrawWorkspaceToolbarItems( UpdateContext const& context ) {}

        // Draws the workspace toolbar menu
        void DrawWorkspaceToolbar( UpdateContext const& context );

        // Called within the context of a large overlay window allowing you to draw helpers and widgets over a viewport
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport ) {}

        // Draw the viewport toolbar
        virtual void DrawViewportToolbarItems( UpdateContext const& context, Render::Viewport const* pViewport ) {}

        // Draw the viewport for this workspace - returns true if this viewport is currently focused
        bool DrawViewport( UpdateContext const& context, ViewportInfo const& viewportInfo, ImGuiWindowClass* pWindowClass );

        // Camera
        //-------------------------------------------------------------------------

        void SetCameraUpdateEnabled( bool isEnabled );

        void ResetCameraView();

        void FocusCameraView( Entity* pTarget );

        void SetViewportCameraSpeed( float cameraSpeed );

        void SetViewportCameraTransform( Transform const& cameraTransform );

        Transform GetViewportCameraTransform() const;

        // Preview World Functions
        //-------------------------------------------------------------------------

        void SetWorldPaused( bool isPaused );

        void SetWorldTimeScale( float newTimeScale );

        void ResetWorldTimeScale();

        // Undo/Redo
        //-------------------------------------------------------------------------

        // Called whenever we execute an undo or redo action
        virtual void PreUndoRedo( UndoStack::Operation operation ) {}
        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction );

        inline bool CanUndo() { return m_undoStack.CanUndo(); }
        void Undo();
        inline bool CanRedo() { return m_undoStack.CanRedo(); }
        void Redo();

        void PreEdit( PropertyEditInfo const& info );
        void PostEdit( PropertyEditInfo const& info );
        void BeginModification();
        void EndModification();

        // Saving and Dirty state
        //-------------------------------------------------------------------------

        // Flag this workspace as dirty
        void MarkDirty() { m_isDirty = true; }

        // Has any modifications been made to this file?
        virtual bool IsDirty() const { return m_isDirty; }

        // Should we always allow saving?
        virtual bool AlwaysAllowSaving() const { return false; }

        // Optional: Save functionality for files that support it
        virtual bool Save();

        // Hot-reload
        //-------------------------------------------------------------------------

        // Are we actually reloading any of our resources
        inline bool IsHotReloading() const { return !m_reloadingResources.empty(); }

        // Called whenever a hot-reload operation occurs
        virtual void BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToBeReloaded, TVector<ResourceID> const& resourcesToBeReloaded );

        // Called after hot-reloading completes
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

        // Descriptor Utils
        //-------------------------------------------------------------------------

        // Draws a separate descriptor property grid editor window - return true if focused
        bool DrawDescriptorEditorWindow( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isSeparateWindow = true );

        template<typename T>
        T* GetDescriptorAs() { return Cast<T>( m_pDescriptor ); }

        virtual void SerializeCustomDescriptorData( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& descriptorObjectValue ) {}
        virtual void SerializeCustomDescriptorData( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) {}

    private:

        Workspace& operator=( Workspace const& ) = delete;
        Workspace( Workspace const& ) = delete;

        void LoadDescriptor();

    protected:

        EntityWorld*                                m_pWorld = nullptr;
        ToolsContext const*                         m_pToolsContext = nullptr;

        DebugCameraComponent*                       m_pCamera = nullptr;

        UndoStack                                   m_undoStack;
        String                                      m_displayName;
        String                                      m_workspaceWindowID;
        String                                      m_viewportWindowID;
        String                                      m_dockspaceID;
        bool                                        m_isViewportFocused = false;
        bool                                        m_isViewportHovered = false;

        String                                      m_descriptorWindowName;
        ResourceID                                  m_descriptorID;
        FileSystem::Path                            m_descriptorPath;
        PropertyGrid*                               m_pDescriptorPropertyGrid = nullptr;
        Resource::ResourceDescriptor*               m_pDescriptor = nullptr;
        EventBindingID                              m_preEditEventBindingID;
        EventBindingID                              m_postEditEventBindingID;
        ResourceDescriptorUndoableAction*           m_pActiveUndoableAction = nullptr;
        int32_t                                     m_beginModificationCallCount = 0;
        ImGuiX::Gizmo                               m_gizmo;

    private:

        bool                                        m_isDirty = false;

        // Time controls
        float                                       m_worldTimeScale = 1.0f;

        // Hot-reloading
        TVector<Resource::ResourcePtr*>             m_requestedResources;
        TVector<Entity*>                            m_addedEntities;
        TVector<Resource::ResourcePtr*>             m_reloadingResources;
    };

    //-------------------------------------------------------------------------

    class [[nodiscard]] ScopedDescriptorModification
    {
    public:

        ScopedDescriptorModification( Workspace* pWorkspace )
            : m_pWorkspace( pWorkspace )
        {
            EE_ASSERT( pWorkspace != nullptr );
            m_pWorkspace->BeginModification();
        }

        virtual ~ScopedDescriptorModification()
        {
            m_pWorkspace->EndModification();
        }

    private:

        Workspace*  m_pWorkspace = nullptr;
    };

    //-------------------------------------------------------------------------
    // Specific Resource Workspace
    //-------------------------------------------------------------------------
    // This is a base class to create a sub-editor for a given resource type that runs within the resource editor

    template<typename T>
    class TWorkspace : public Workspace
    {
        static_assert( std::is_base_of<Resource::IResource, T>::value, "T must derived from IResource" );

    public:

        // Specify whether to initially load the resource, this is not necessary for all editors
        TWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID, bool shouldLoadResource = true )
            : Workspace( pToolsContext, pWorld, resourceID )
            , m_pResource( resourceID )
        {
            EE_ASSERT( resourceID.IsValid() );

            if ( shouldLoadResource )
            {
                LoadResource( &m_pResource );
            }
        }

        ~TWorkspace()
        {
            if ( !m_pResource.IsUnloaded() )
            {
                UnloadResource( &m_pResource );
            }
        }

        virtual uint32_t GetID() const override { return m_pResource.GetResourceID().GetPathID(); }
        virtual bool HasViewportWindow() const override { return true; }
        virtual bool HasViewportToolbar() const { return true; }

        // Resource Status
        inline bool IsLoading() const { return m_pResource.IsLoading(); }
        inline bool IsUnloaded() const { return m_pResource.IsUnloaded(); }
        inline bool IsResourceLoaded() const { return m_pResource.IsLoaded(); }
        inline bool IsWaitingForResource() const { return IsLoading() || IsUnloaded() || m_pResource.IsUnloading(); }
        inline bool HasLoadingFailed() const { return m_pResource.HasLoadingFailed(); }

    protected:

        TResourcePtr<T>                     m_pResource;
    };

    //-------------------------------------------------------------------------
    // Resource Workspace Factory
    //-------------------------------------------------------------------------
    // Used to spawn the appropriate factory

    class EE_ENGINETOOLS_API ResourceWorkspaceFactory : public TGlobalRegistryBase<ResourceWorkspaceFactory>
    {
        EE_DECLARE_GLOBAL_REGISTRY( ResourceWorkspaceFactory );

    public:

        static bool CanCreateWorkspace( ToolsContext const* pToolsContext, ResourceID const& resourceID );
        static Workspace* CreateWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID );

    protected:

        // Get the resource type this factory supports
        virtual ResourceTypeID GetSupportedResourceTypeID() const = 0;

        // Virtual method that will create a workspace if the resource ID matches the appropriate types
        virtual Workspace* CreateWorkspaceInternal( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID ) const = 0;
    };

    //-------------------------------------------------------------------------
    //  Macro to create a resource workspace factory
    //-------------------------------------------------------------------------
    // Use in a CPP to define a factory e.g., EE_RESOURCE_WORKSPACE_FACTORY( SkeletonWorkspaceFactory, Skeleton, SkeletonResourceEditor );

    #define EE_RESOURCE_WORKSPACE_FACTORY( factoryName, resourceClass, workspaceClass )\
    class factoryName final : public ResourceWorkspaceFactory\
    {\
        virtual ResourceTypeID GetSupportedResourceTypeID() const override { return resourceClass::GetStaticResourceTypeID(); }\
        virtual Workspace* CreateWorkspaceInternal( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID ) const override\
        {\
            EE_ASSERT( resourceID.GetResourceTypeID() == resourceClass::GetStaticResourceTypeID() );\
            return EE::New<workspaceClass>( pToolsContext, pWorld, resourceID );\
        }\
    };\
    static factoryName g_##factoryName;
}