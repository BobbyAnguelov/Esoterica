#pragma once
#include "UndoStack.h"
#include "ToolsContext.h"
#include "PropertyGrid/PropertyGrid.h"
#include "EngineTools/Resource/ResourceDatabase.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Imgui/ImguiGizmo.h"
#include "Base/Render/RenderTarget.h"
#include "Base/Resource/ResourceRequesterID.h"
#include "Base/Resource/ResourcePtr.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Utils/GlobalRegistryBase.h"
#include "Base/Serialization/JsonSerialization.h"
#include "Base/Drawing/DebugDrawing.h"
#include "Base/Types/Function.h"

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
// A workspace is a set of tools used to create/edit an engine resource
// Assumes you will have some file that you read/write to and an entity world for preview.
// Provides a lot of utility functions for loading resource/creating entities that must be used!

namespace EE
{
    class EE_ENGINETOOLS_API Workspace
    {
        friend class EditorUI;
        friend class ScopedDescriptorModification;
        friend ResourceDescriptorUndoableAction;

        // Create the tool window name
        EE_FORCE_INLINE static InlineString GetToolWindowName( char const* pToolWindowName, ImGuiID dockspaceID )
        {
            EE_ASSERT( pToolWindowName != nullptr );
            return InlineString( InlineString::CtorSprintf(), "%s##%08X", pToolWindowName, dockspaceID );
        }

    public:

        struct ViewportInfo
        {
            ImTextureID                                     m_pViewportRenderTargetTexture = nullptr;
            TFunction<Render::PickingID( Int2 const& )>     m_retrievePickingID;
        };

        class ToolWindow
        {
            friend class Workspace;
            friend class EditorUI;

            enum class Type : uint8_t
            {
                Viewport,
                Descriptor,
                Tool
            };

        public:

            ToolWindow( String const& name, TFunction<void( UpdateContext const&, bool )> const& drawFunction, ImVec2 const& windowPadding = ImVec2( -1, -1 ) )
                : m_name( name )
                , m_drawFunction( drawFunction )
                , m_windowPadding( windowPadding )
            {
                EE_ASSERT( !name.empty() );
                EE_ASSERT( m_drawFunction != nullptr );
            }

            inline bool HasUserSpecifiedWindowPadding() const
            {
                return m_windowPadding.x >= 0 && m_windowPadding.y >= 0;
            }

        private:

            String                                          m_name;
            TFunction<void(UpdateContext const&, bool)>     m_drawFunction;
            ImVec2                                          m_windowPadding;
            Type                                            m_type = Type::Tool;
            bool                                            m_isOpen = true;
        };

    public:

        explicit Workspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID );
        Workspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, String const& displayName );
        virtual ~Workspace();

        // Workspace
        //-------------------------------------------------------------------------

        // Get the display name for this workspace (shown on tab, dialogs, etc...)
        inline char const* GetDisplayName() const { return m_windowName.c_str(); }

        // Does this workspace have a title bar icon
        virtual bool HasTitlebarIcon() const { return false; }

        // Get this workspace's title bar icon
        virtual char const* GetTitlebarIcon() const { EE_ASSERT( HasTitlebarIcon() ); return nullptr; }

        // Does this workspace have a toolbar?
        virtual bool HasMenu() const { return true; }

        // Draws the workspace toolbar menu - by default will draw the default items and descriptor items
        virtual void DrawMenu( UpdateContext const& context );

        // Does this workspace operate on a resource descriptor?
        inline bool IsADescriptorWorkspace() const { return m_descriptorID.IsValid(); }

        // Has the descriptor been loaded (only valid for descriptor workspaces)
        inline bool IsDescriptorLoaded() const { return m_pDescriptor != nullptr; }

        // Was the initialization function called
        inline bool IsInitialized() const { return m_isInitialized; }

        // Draw any open dialogs that this workspace has
        virtual void DrawDialogs( UpdateContext const& context ) {}

        // Viewport
        //-------------------------------------------------------------------------

        // Should this workspace display a viewport?
        virtual bool HasViewportWindow() const { return false; }

        // Should we draw the time control widget in the viewport toolbar
        virtual bool HasViewportToolbarTimeControls() const { return false; }

        // Does this workspace's viewport have a orientation guide drawn?
        virtual bool HasViewportOrientationGuide() const { return true; }

        // Called within the context of a large overlay window allowing you to draw helpers and widgets over a viewport
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport ) {}

        // Draw the viewport for this workspace - returns true if this viewport is currently focused
        void DrawViewport( UpdateContext const& context, ViewportInfo const& viewportInfo );

        // Docking
        //-------------------------------------------------------------------------

        // Get the unique typename for this workspace to be used for docking
        virtual char const* GetDockingUniqueTypeName() const = 0;

        // Set up initial docking layout
        virtual void InitializeDockingLayout( ImGuiID const dockspaceID, ImVec2 const& dockspaceSize ) const;

        // Resource Dependencies
        //-------------------------------------------------------------------------

        // Is this workspace currently working on this resource. This is necessary since we have situations where multiple resourceIDs all map to the same workspace.
        // For example, all graph variations should open the workspace for the parent graph.
        virtual bool IsWorkingOnResource( ResourceID const& resourceID ) const { return resourceID == m_descriptorID; }

        // Is this resource necessary for this workspace to function. Primarily needed to determine when to automatically close workspaces due to external resource deletion.
        // Again we need potential derivations due to cases where multiple resources tie into a single workspace.
        virtual bool HasDependencyOnResource( ResourceID const& resourceID ) const { return resourceID == m_descriptorID; }

        // Lifetime/Update Functions
        //-------------------------------------------------------------------------

        // Initialize the workspace: initialize window IDs, create preview entities, load descriptor etc...
        // NOTE: Derived classes MUST call base implementation!
        virtual void Initialize( UpdateContext const& context );

        // Shutdown the workspace, free loaded descriptor, etc...
        virtual void Shutdown( UpdateContext const& context );

        // Called just before the world is updated per update stage
        virtual void PreUpdateWorld( EntityWorldUpdateContext const& updateContext ) {}

        // Called by the editor before the main update, this handles a lot of the shared functionality (undo/redo/etc...)
        void SharedUpdate( UpdateContext const& context, bool isVisible, bool isFocused );

        // Frame update and draw any tool windows needed for the workspace
        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) {}

        // Preview World
        //-------------------------------------------------------------------------

        // Should we load the default editor map for this workspace?
        virtual bool ShouldLoadDefaultEditorMap() const { return true; }

        // Get the world associated with this workspace
        inline EntityWorld* GetWorld() const { return m_pWorld; }

        void SetWorldPaused( bool isPaused );

        void SetWorldTimeScale( float newTimeScale );

        void ResetWorldTimeScale();

        // Camera
        //-------------------------------------------------------------------------

        void SetCameraUpdateEnabled( bool isEnabled );

        void ResetCameraView();

        void FocusCameraView( Entity* pTarget );

        void SetCameraSpeed( float cameraSpeed );

        void SetCameraTransform( Transform const& cameraTransform );

        Transform GetCameraTransform() const;

        // Undo/Redo
        //-------------------------------------------------------------------------

        // Called immediately before we execute an undo or redo action - undo/redo commands occur before the workspace "update" call
        virtual void PreUndoRedo( UndoStack::Operation operation ) {}

        // Called immediately after we execute an undo or redo action - undo/redo commands occur before the workspace "update" call
        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) {}

        inline bool CanUndo() { return m_undoStack.CanUndo(); }
        void Undo();

        inline bool CanRedo() { return m_undoStack.CanRedo(); }
        void Redo();

        // Saving and Dirty state
        //-------------------------------------------------------------------------

        // Flag this workspace as dirty
        void MarkDirty() { m_isDirty = true; }

        // Clear the dirty flag
        void ClearDirty() { m_isDirty = false; }

        // Has any modifications been made to this file?
        bool IsDirty() const { return m_isDirty; }

        // Should we always allow saving?
        virtual bool AlwaysAllowSaving() const { return false; }

        // Optional: Save functionality for files that support it
        virtual bool Save();

        // Hot-reload
        //-------------------------------------------------------------------------

        // Called whenever a hot-reload operation occurs
        virtual void BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToBeReloaded, TVector<ResourceID> const& resourcesToBeReloaded );

        // Called after hot-reloading completes.
        virtual void EndHotReload();

    protected:

        // Called whenever we click inside a workspace viewport and get a non-zero picking ID
        virtual void OnMousePick( Render::PickingID pickingID ) {}

        // Helper function for debug drawing
        Drawing::DrawContext GetDrawingContext();

        // Set the workspace tab-title
        void SetDisplayName( String name );

        // Tool Windows
        //-------------------------------------------------------------------------

        ImGuiWindowClass* GetToolWindowClass() { return &m_toolWindowClass; }

        void CreateToolWindow( String const& name, TFunction<void( UpdateContext const&, bool )> const& drawFunction, ImVec2 const& windowPadding = ImVec2( -1, -1 ) );

        EE_FORCE_INLINE TVector<ToolWindow> const& GetToolWindows() const { return m_toolWindows; }

        EE_FORCE_INLINE InlineString GetToolWindowName( String const& name ) const { return GetToolWindowName( name.c_str(), m_currentDockspaceID ); }

        void HideDescriptorWindow();

        // Viewport
        //-------------------------------------------------------------------------

        // Called during the viewport drawing allowing the workspaces to handle drag and drop requests
        virtual void OnDragAndDropIntoViewport( Render::Viewport* pViewport ) {}

        // Called whenever we have a valid resource being dropped into the viewport, users can override to provide custom handling
        virtual void DropResourceInViewport( ResourceID const& resourceID, Vector const& worldPosition ) {}

        // Draws the viewport toolbar
        virtual void DrawViewportToolbar( UpdateContext const& context, Render::Viewport const* pViewport );

        // Begin a toolbar group - You need to call end whether or not this returns true (match imgui child window pattern)
        bool BeginViewportToolbarGroup( char const* pGroupID, ImVec2 groupSize, ImVec2 const& padding = ImVec2( 4.0f, 4.0f ) );

        // End a toolbar group, always call this if you called "BeginViewportToolbarGroup"
        void EndViewportToolbarGroup();

        // Creates a viewport drop down 
        void DrawViewportToolbarCombo( char const* pID, char const* pLabel, char const* pTooltip, TFunction<void()> const& function, float width = -1 );

        // Creates a fixed width viewport drop down with an icon as a label
        void DrawViewportToolbarComboIcon( char const* pID, char const* pIcon, char const* pTooltip, TFunction<void()> const& function );

        // Draw the common viewport toolbar items (rendering/camera/etc...)
        void DrawViewportToolBarCommon();

        // Draw the viewport time controls toolbar
        void DrawViewportToolBar_TimeControls();

        // Resource Helpers
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

        // Use this function to load a resource required for this workspace (hot-reload aware)
        void LoadResource( Resource::ResourcePtr* pResourcePtr );

        // Use this function to unload a required resource for this workspace (hot-reload aware)
        void UnloadResource( Resource::ResourcePtr* pResourcePtr );

        // Called before we start a hot-reload operation (before we actually unload anything, allows workspaces to clean up any dependent data)
        // This is only called if any of the resources that workspace has explicitly loaded (or the descriptor) needs a reload
        virtual void OnHotReloadStarted( bool descriptorNeedsReload, TInlineVector<Resource::ResourcePtr*, 10> const& resourcesToBeReloaded ) {}

        // Called after hot-reloading completes (you will always get this call if you get the 'OnHotReloadStarted' call, unless a fatal occurs)
        // This only gets called if we needed to reload anything and the reload was successful
        // If the descriptor fails to reload, this is considered a fatal error and this function will not be called and the workspace will be closed instead
        virtual void OnHotReloadComplete() {}

        // Blocking call to force a recompile of a resource outside of the regular resource server flow, this is needed when you need to ensure a resource has been recompiled in advance of loading
        // Will also flag the resource for hot-reload if there are any current users
        bool RequestImmediateResourceCompilation( ResourceID const& resourceID );

        // Entity Helpers
        //-------------------------------------------------------------------------

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

        // Get the descriptor as a derived type (Note! You need to check if the descriptor is actually loaded)
        template<typename T>
        T* GetDescriptor() { return Cast<T>( m_pDescriptor ); }

        // Get the descriptor as a derived type (Note! You need to check if the descriptor is actually loaded)
        template<typename T>
        T const* GetDescriptor() const { return Cast<T>( m_pDescriptor ); }

        // This is called to allow you to read custom data from the descriptor JSON data
        virtual void ReadCustomDescriptorData( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& descriptorObjectValue ) {}

        // This is called to allow you to write custom data into the descriptor JSON data
        virtual void WriteCustomDescriptorData( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) {}

        // Call this immediately before you change the descriptor data (this serializes the original state of the descriptor state into the undo-redo-stack)
        void BeginDescriptorModification();

        // Call this once you are finished modifying the descriptor state (this serializes the modified state of the descriptor into the undo-redo-stack)
        void EndDescriptorModification();

        // Draws a separate descriptor property grid editor window - return true if focused
        void DrawDescriptorEditorWindow( UpdateContext const& context, bool isFocused );

private:

        Workspace& operator=( Workspace const& ) = delete;
        Workspace( Workspace const& ) = delete;

        // Loads the specified descriptor in the descriptor ID member
        void LoadDescriptor();

        // Create the workspace camera
        void CreateCamera();

        // Calculate the dockspace ID for this workspace relative to its parent
        inline ImGuiID CalculateDockspaceID()
        {
            int32_t dockspaceID = m_currentLocationID;
            char const* const pWorkspaceTypeName = GetDockingUniqueTypeName();
            dockspaceID = ImHashData( pWorkspaceTypeName, strlen( pWorkspaceTypeName ), dockspaceID );
            return dockspaceID;
        }

    protected:

        ImGuiID                                     m_ID = 0; // Document identifier (unique)
        EntityWorld*                                m_pWorld = nullptr;
        ToolsContext const*                         m_pToolsContext = nullptr;

        DebugCameraComponent*                       m_pCamera = nullptr;

        UndoStack                                   m_undoStack;
        String                                      m_windowName;
        bool                                        m_isViewportFocused = false;
        bool                                        m_isViewportHovered = false;

        ResourceID                                  m_descriptorID;
        FileSystem::Path                            m_descriptorPath;
        PropertyGrid*                               m_pDescriptorPropertyGrid = nullptr;
        Resource::ResourceDescriptor*               m_pDescriptor = nullptr;
        EventBindingID                              m_preEditEventBindingID;
        EventBindingID                              m_postEditEventBindingID;
        IUndoableAction*                            m_pActiveUndoableAction = nullptr;
        int32_t                                     m_beginModificationCallCount = 0;
        ImGuiX::Gizmo                               m_gizmo;
        bool                                        m_isDescriptorWindowFocused = false;

    private:

        ImGuiID                                     m_currentDockID = 0;        // The doc we are currently in
        ImGuiID                                     m_desiredDockID = 0;        // The dock we wish to be in
        ImGuiID                                     m_currentLocationID = 0;    // Current Dock node we are docked into _OR_ window ID if floating window
        ImGuiID                                     m_previousLocationID = 0;   // Previous dock node we are docked into _OR_ window ID if floating window
        ImGuiID                                     m_currentDockspaceID = 0;   // Dockspace ID ~~ Hash of LocationID + WorkspaceType
        ImGuiID                                     m_previousDockspaceID = 0;
        TVector<ToolWindow>                         m_toolWindows;
        ImGuiWindowClass                            m_toolWindowClass;          // All our tools windows will share the same WindowClass (based on ID) to avoid mixing tools from different top-level window

        Resource::ResourceSystem*                   m_pResourceSystem = nullptr;
        bool                                        m_isDirty = false;
        bool                                        m_isInitialized = false;
        bool                                        m_isHotReloading = false;

        // Time controls
        float                                       m_worldTimeScale = 1.0f;

        // Hot-reloading
        TVector<Resource::ResourcePtr*>             m_requestedResources;
        TVector<Entity*>                            m_addedEntities;
        TVector<Resource::ResourcePtr*>             m_reloadingResources;
    };

    //-------------------------------------------------------------------------

    class ResourceDescriptorUndoableAction final : public IUndoableAction
    {
        EE_REFLECT_TYPE( IUndoableAction );

    public:

        ResourceDescriptorUndoableAction() = default;
        ResourceDescriptorUndoableAction( TypeSystem::TypeRegistry const* pTypeRegistry, Workspace* pWorkspace );

        virtual void Undo() override;
        virtual void Redo() override;
        void SerializeBeforeState();
        void SerializeAfterState();

    private:

        TypeSystem::TypeRegistry const*     m_pTypeRegistry = nullptr;
        Workspace*                          m_pWorkspace = nullptr;
        String                              m_valueBefore;
        String                              m_valueAfter;
    };

    //-------------------------------------------------------------------------

    class [[nodiscard]] ScopedDescriptorModification
    {
    public:

        ScopedDescriptorModification( Workspace* pWorkspace )
            : m_pWorkspace( pWorkspace )
        {
            EE_ASSERT( pWorkspace != nullptr );
            m_pWorkspace->BeginDescriptorModification();
        }

        virtual ~ScopedDescriptorModification()
        {
            m_pWorkspace->EndDescriptorModification();
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
        TWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID, bool shouldAutoLoadResource = true )
            : Workspace( pToolsContext, pWorld, resourceID )
            , m_workspaceResource( resourceID )
            , m_shouldAutoLoadResource( shouldAutoLoadResource )
        {
            EE_ASSERT( resourceID.IsValid() );
        }

        virtual void Initialize( UpdateContext const& context ) override
        {
            Workspace::Initialize( context );

            if ( m_shouldAutoLoadResource )
            {
                LoadResource( &m_workspaceResource );
            }
        }

        virtual void Shutdown( UpdateContext const& context ) override
        {
            if ( m_workspaceResource.WasRequested() )
            {
                UnloadResource( &m_workspaceResource );
            }

            Workspace::Shutdown( context );
        }

        virtual bool HasViewportWindow() const override { return true; }

        // Resource Status
        inline bool IsLoading() const { return m_workspaceResource.IsLoading(); }
        inline bool IsUnloaded() const { return m_workspaceResource.IsUnloaded(); }
        inline bool IsResourceLoaded() const { return m_workspaceResource.IsLoaded(); }
        inline bool IsWaitingForResource() const { return IsLoading() || IsUnloaded() || m_workspaceResource.IsUnloading(); }
        inline bool HasLoadingFailed() const { return m_workspaceResource.HasLoadingFailed(); }

    protected:

        TResourcePtr<T>                     m_workspaceResource;
        bool                                m_shouldAutoLoadResource = true;
    };

    //-------------------------------------------------------------------------
    // Default Generic Workspace
    //-------------------------------------------------------------------------

    class GenericWorkspace : public Workspace
    {
    public:

        using Workspace::Workspace;
        virtual char const* GetDockingUniqueTypeName() const override { return "Descriptor"; }
    };

    //-------------------------------------------------------------------------
    // Workspace Factory
    //-------------------------------------------------------------------------
    // Used to spawn the appropriate workspace

    class EE_ENGINETOOLS_API ResourceWorkspaceFactory : public TGlobalRegistryBase<ResourceWorkspaceFactory>
    {
        EE_DECLARE_GLOBAL_REGISTRY( ResourceWorkspaceFactory );

    public:

        virtual ~ResourceWorkspaceFactory() = default;

        static bool CanCreateWorkspace( ToolsContext const* pToolsContext, ResourceID const& resourceID );
        static Workspace* CreateWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID );

    protected:

        // Get the resource type this factory supports
        virtual ResourceTypeID GetSupportedResourceTypeID() const { return ResourceTypeID(); }

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