#pragma once

#include "EditorTool.h"
#include "Base/Render/RenderTarget.h"
#include "Base/Resource/ResourceRequesterID.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Serialization/JsonSerialization.h"
#include "Base/Drawing/DebugDrawing.h"
#include "Base/Types/Function.h"

//-------------------------------------------------------------------------

namespace EE
{
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

// Note:    Workspace uses editor-tool as a base since there is a lot of shared functionality but workspaces ARE NOT editor tools.
//          There are various distinctions in terms of creation/destruction and so on.

namespace EE
{
    class EE_ENGINETOOLS_API Workspace : public EditorTool
    {
        friend class EditorUI;
        friend class ScopedDescriptorModification;
        friend ResourceDescriptorUndoableAction;
        template<class T> friend class TWorkspace;

    public:

        struct ViewportInfo
        {
            ImTextureID                                     m_pViewportRenderTargetTexture = nullptr;
            TFunction<Render::PickingID( Int2 const& )>     m_retrievePickingID;
        };

    public:

        explicit Workspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID );
        Workspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, String const& displayName );
        virtual ~Workspace();

        // Workspace
        //-------------------------------------------------------------------------

        // Does this workspace have a toolbar?
        virtual bool HasMenu() const override { return true; }

        // Draws the workspace toolbar menu - by default will draw the default items and descriptor items
        virtual void DrawMenu( UpdateContext const& context ) override;

        // Does this workspace operate on a resource descriptor?
        inline bool IsADescriptorWorkspace() const { return m_descriptorID.IsValid(); }

        // Has the descriptor been loaded (only valid for descriptor workspaces)
        inline bool IsDescriptorLoaded() const { return m_pDescriptor != nullptr; }

        // Docking
        //-------------------------------------------------------------------------

        // Set up initial docking layout
        virtual void InitializeDockingLayout( ImGuiID const dockspaceID, ImVec2 const& dockspaceSize ) const override;

        // Viewport
        //-------------------------------------------------------------------------

        // Should this workspace display a viewport?
        virtual bool HasViewportWindow() const { return false; }

        // Should we draw the time control widget in the viewport toolbar
        virtual bool HasViewportToolbarTimeControls() const { return false; }

        // Does this workspace's viewport have a orientation guide drawn?
        virtual bool HasViewportOrientationGuide() const { return true; }

        // A large viewport overlay window is created for all workspaces allowing you to draw helpers and widgets over a viewport
        // Called as part of the workspace drawing so at "Frame Start" - so be careful of any timing issue with 3D drawing
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport ) {}

        // Draw the viewport for this workspace - returns true if this viewport is currently focused
        void DrawViewport( UpdateContext const& context, ViewportInfo const& viewportInfo );

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
        virtual void Initialize( UpdateContext const& context ) override;

        // Called just before the world is updated per update stage
        virtual void PreWorldUpdate( EntityWorldUpdateContext const& updateContext ) {}

        // Called by the editor before the main update, this handles a lot of the shared functionality (undo/redo/etc...)
        void SharedUpdate( UpdateContext const& context, bool isVisible, bool isFocused );

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
        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction );

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

    protected:

        // Called whenever we click inside a workspace viewport and get a non-zero picking ID
        virtual void OnMousePick( Render::PickingID pickingID ) {}

        // Helper function for debug drawing
        Drawing::DrawContext GetDrawingContext();

        // Set the workspace tab-title
        virtual void SetDisplayName( String name ) override final;

        // Tool Windows
        //-------------------------------------------------------------------------

        void HideDescriptorWindow();

        // Resource Helpers
        //-------------------------------------------------------------------------

        // Called before we start a hot-reload operation (before we actually unload anything, allows workspaces to clean up any dependent data)
        // This is only called if any of the resources that workspace has explicitly loaded (or the descriptor) needs a reload
        virtual void OnHotReloadStarted( bool descriptorNeedsReload, TInlineVector<Resource::ResourcePtr*, 10> const& resourcesToBeReloaded ) {}

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

        // Editor Tool
        //-------------------------------------------------------------------------

        virtual uint32_t GetToolTypeID() const override final { return 0xFFFFFFFF; }
        virtual bool IsSingleton() const override final { return false; }
        virtual bool IsSingleWindowTool() const override final { return false; }
        virtual void OnHotReloadStarted( TInlineVector<Resource::ResourcePtr*, 10> const& resourcesToBeReloaded ) override final {}
        virtual void HotReload_UnloadResources( TVector<Resource::ResourceRequesterID> const& usersToBeReloaded, TVector<ResourceID> const& resourcesToBeReloaded ) override final;
        virtual void HotReload_ReloadComplete() override final;

        // Workspace internals
        //-------------------------------------------------------------------------

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

        // For TWorkspaces
        virtual void PrivateInternalUpdate( UpdateContext const& context ) {}

    protected:

        EntityWorld*                                m_pWorld = nullptr;
        DebugCameraComponent*                       m_pCamera = nullptr;

        UndoStack                                   m_undoStack;
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

        bool                                        m_isDirty = false;

        // Time controls
        float                                       m_worldTimeScale = 1.0f;

        // Hot-reloading
        TVector<Entity*>                            m_addedEntities;
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
        //-------------------------------------------------------------------------

        inline bool IsLoading() const { return m_workspaceResource.IsLoading(); }
        inline bool IsUnloaded() const { return m_workspaceResource.IsUnloaded(); }
        inline bool IsResourceLoaded() const { return m_workspaceResource.IsLoaded(); }
        inline bool IsWaitingForResource() const { return IsLoading() || IsUnloaded() || m_workspaceResource.IsUnloading(); }
        inline bool HasLoadingFailed() const { return m_workspaceResource.HasLoadingFailed(); }

    protected:

        // Called the first time we load this resource - all subsequent re-loads need to be handled via the hot-reload path
        virtual void OnInitialResourceLoadCompleted() {}

    private:

        virtual void PrivateInternalUpdate( UpdateContext const& context ) final
        {
            if ( !m_hasCompletedInitialLoad )
            {
                if ( IsResourceLoaded() || HasLoadingFailed() )
                {
                    OnInitialResourceLoadCompleted();
                    m_hasCompletedInitialLoad = true;
                }
            }
        }

    protected:

        TResourcePtr<T>                     m_workspaceResource;
        bool                                m_shouldAutoLoadResource = true;

    private:

        bool                                m_hasCompletedInitialLoad = false;
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