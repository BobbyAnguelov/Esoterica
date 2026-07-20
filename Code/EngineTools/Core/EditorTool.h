#pragma once
#include "UndoStack.h"
#include "ToolsContext.h"
#include "EngineTools/PropertyGrid/PropertyGrid.h"
#include "EngineTools/FileSystem/FileRegistry.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Viewport/Viewport.h"
#include "Base/Utils/GlobalRegistryBase.h"
#include "Base/Resource/ResourcePtr.h"
#include "Base/Drawing/DebugDrawing.h"
#include "Base/Time/Timers.h"

//-------------------------------------------------------------------------

namespace EE
{
    class UpdateContext;
    class Entity;
    class EntityWorld;
    class EntityWorldUpdateContext;
    class ToolsCameraComponent;
    class DataFileUndoableAction;

    namespace Render
    {
        class StaticMeshComponent;
        class DirectionalLightComponent;
    }

    namespace Physics
    {
        class CollisionMeshComponent;
    }
}

//-------------------------------------------------------------------------
// Editor Tool
//-------------------------------------------------------------------------
// An editor tool is a host for a set of UI tools to provide information about the state of the engine
// or to edit various engine settings and resources.
//
// Provides a lot of utility functions for loading resource/creating entities that must be used!

namespace EE
{
    class EE_ENGINETOOLS_API EditorTool
    {
        friend class EditorUI;

        constexpr static char const* const s_viewportWindowName = "Viewport";

    public:

        constexpr static char const* const s_dataFileWindowName = "Data File";

        class ToolWindow final
        {
            friend class EditorTool;
            friend class EditorUI;

        public:

            ToolWindow( String const& name, TFunction<void( UpdateContext const&, bool )> const& drawFunction, ImVec2 const& windowPadding = ImVec2( -1, -1 ), bool disableScrolling = false )
                : m_name( name )
                , m_drawFunction( drawFunction )
                , m_windowPadding( windowPadding )
                , m_disableScrolling( disableScrolling )
            {
                EE_ASSERT( !name.empty() );
                EE_ASSERT( m_drawFunction != nullptr );
            }

            inline String const& GetName() const { return m_name; }

            inline bool HasUserSpecifiedWindowPadding() const
            {
                return m_windowPadding.x >= 0 && m_windowPadding.y >= 0;
            }

            inline bool IsScrollingDisabled() const
            {
                return m_disableScrolling;
            }

            ToolWindow& SetIsHiddenFunction( TFunction<bool()>&& func )
            {
                m_isHiddenFunction = func;
                return *this;
            }

            inline bool IsHidden() const
            {
                if ( m_isHiddenFunction != nullptr )
                {
                    return m_isHiddenFunction();
                }

                return false;
            }

            inline void SetOpen( bool bIsOpen ) { m_isOpen = bIsOpen; }

        private:

            String                                          m_name;
            TFunction<void( UpdateContext const&, bool )>   m_drawFunction = nullptr;
            TFunction<bool()>                               m_isHiddenFunction = nullptr;
            ImVec2                                          m_windowPadding;
            bool                                            m_disableScrolling = false;
            Viewport*                                       m_pViewport = nullptr;
            bool                                            m_isOpen = true;
        };

    protected:

        // Create the tool window name
        EE_FORCE_INLINE static InlineString GetToolWindowName( char const* pToolWindowName, ImGuiID dockspaceID )
        {
            EE_ASSERT( pToolWindowName != nullptr );
            return InlineString( InlineString::CtorSprintf(), "%s##%08X", pToolWindowName, dockspaceID );
        }

    private:

        enum class PreviewState
        {
            None,
            Requested,
            WaitForResourceLoad,
            SettingUp,
            Previewing,
            TearingDown
        };

    public:

        explicit EditorTool( ToolsContext const* pToolsContext, String const& displayName, EntityWorld* pWorld = nullptr );
        explicit EditorTool( ToolsContext const* pToolsContext, char const* const pDisplayName, EntityWorld* pWorld = nullptr ) : EditorTool( pToolsContext, String( pDisplayName ), pWorld ) {}
        virtual ~EditorTool();

        // Info
        //-------------------------------------------------------------------------

        // Get the overall unique ID for this tool
        inline uint64_t GetID() const { return m_ID; }

        // Get the hash of the unique type ID for this tool
        virtual uint64_t GetUniqueTypeID() const = 0;

        // Get the unique typename for this tool to be used for docking
        virtual char const* GetUniqueTypeName() const = 0;

        // Get a descriptive name for this tool that includes the type and the file open if applicable
        virtual InlineString GetName() const { return GetUniqueTypeName(); }

        // Was the base initialization function called
        inline bool WasInitialized() const { return m_wasInitializedCalled; }

        // Is this tool currently working on this resource. This is necessary since we have situations where multiple resourceIDs all map to the same tool.
        // For example, all graph variations should open the tool for the parent graph.
        virtual bool IsEditingFile( DataPath const& dataPath ) const { return false; }

        // Is this resource necessary for this tool to function. Primarily needed to determine when to automatically close resource editors due to external resource deletion.
        // Again we need potential derivations due to cases where multiple resources tie into a single tool.
        virtual bool HasDependencyOnFile( DataPath const& dataPath ) const { return false; }

        // Is Singleton tool - i.e. is only one instance of this tool allowed
        virtual bool IsSingleton() const { return false; }

        // Is this tool a single window tool? i.e. no dockspace, only one tool window, etc...
        virtual bool IsSingleWindowTool() const { return false; }

        // Is this a data file editor?
        virtual bool IsDataFileTool() const { return false; }

        // Get the display name for this tool (shown on tab, dialogs, etc...)
        inline char const* GetDisplayName() const { return m_windowName.c_str(); }

        // Does this tool have a title bar icon
        virtual bool HasTitlebarIcon() const { return false; }

        // Get this tool's title bar icon
        virtual char const* GetTitlebarIcon() const { EE_ASSERT( HasTitlebarIcon() ); return nullptr; }

        // Lifetime/Update Functions
        //-------------------------------------------------------------------------

        // Initialize the tool: initialize window IDs, load data, etc...
        // NOTE: Derived classes MUST call base implementation!
        virtual void Initialize( UpdateContext const& context );

        // Shutdown the tool, free allocated memory, etc...
        virtual void Shutdown( UpdateContext const& context );

        // Pre-Draw update - Called before we draw all the tool windows
        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) {}

        // Called just before the world is updated for each update stage - this is called after the tool update and finalize
        virtual void WorldUpdate( EntityWorldUpdateContext const& updateContext ) {}

        // Menu and Help
        //-------------------------------------------------------------------------

        // Does this tool have a main menu?
        virtual bool SupportsMainMenu() const { return true; }

        // Draws the tool main menu
        virtual void DrawMenu( UpdateContext const& context ) {}

        // Toolbar
        //-------------------------------------------------------------------------

        // Does this tool have an additional toolbar?
        virtual bool SupportsToolbar() const { return false; }

        // Draws the tool toolbar menu
        virtual void DrawToolbar( UpdateContext const& context ) {}

        // New file creation
        //-------------------------------------------------------------------------

        // Do we support creating a new file from this tool?
        virtual bool SupportsNewFileCreation() const { return false; }

        // This function is called whenever the user requests a new file to be created
        virtual void CreateNewFile() const { EE_ASSERT( SupportsNewFileCreation() ); }

        // Undo/Redo
        //-------------------------------------------------------------------------

        inline bool HasActiveUndoableAction() const { return m_pActiveUndoableAction != nullptr; }

        // Does this tool support saving
        virtual bool SupportsUndoRedo() const { return true; }

        // Called immediately before we execute an undo or redo action - undo/redo commands occur before the tool "update" call
        virtual void PreUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) {}

        // Called immediately after we execute an undo or redo action - undo/redo commands occur before the tool "update" call
        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction );

        inline bool CanUndo() { return m_undoStack.CanUndo(); }
        void Undo();

        inline bool CanRedo() { return m_undoStack.CanRedo(); }
        void Redo();

        // Saving and Dirty state
        //-------------------------------------------------------------------------

        // Does this tool support saving
        virtual bool SupportsSaving() const { return false; }

        // Flag this tool as dirty
        virtual void MarkDirty() { m_isDirty = true; }

        // Clear the dirty flag
        virtual void ClearDirty() { m_isDirty = false; }

        // Has any modifications been made to this file?
        bool IsDirty() const { return m_isDirty; }

        // Should we always allow saving?
        virtual bool AlwaysAllowSaving() const { return false; }

        // Get the target filename for the save operation
        virtual String GetFilenameForSave() const { return ""; }

        // Call this function to save! Returns true if successful
        bool Save();

        // Entity World
        //-------------------------------------------------------------------------

        // Does this tool have a preview world
        inline bool HasEntityWorld() const { return m_pWorld != nullptr; }

        // Get the world associated with this tool
        inline EntityWorld* GetEntityWorld() const { return m_pWorld; }

        void SetWorldPaused( bool isPaused );

        void SetWorldTimeScale( float newTimeScale );

        void ResetWorldTimeScale();

        // Should we draw the time control widget in the viewport toolbar
        virtual bool SupportsWorldTimeControls() const { return false; }

        // Default Preview Map
        //-------------------------------------------------------------------------

        // Should we load the default editor map for this tool?
        virtual bool ShouldLoadDefaultEditorMap() const { return true; }

        // Set the visibility of the default editor map floor
        void SetFloorVisibility( bool showFloorMesh );

        // Is the default editor map floor visible
        bool GetFloorVisibility() const;

        // Viewport
        //-------------------------------------------------------------------------

        // Should this tool display a viewport?
        virtual bool SupportsViewport() const { return m_pWorld != nullptr; }

        // Does this tool's viewport have a orientation guide drawn?
        virtual bool HasViewportOrientationGuide() const { return true; }

        // A large viewport overlay window is created for all editor tools allowing you to draw helpers and widgets over a viewport
        // Called as part of the tool drawing so at "Frame Start" - so be careful of any timing issue with 3D drawing
        virtual void DrawViewportUI( UpdateContext const& context, Viewport const* pViewport, bool isFocused ) {}

        // Camera
        //-------------------------------------------------------------------------

        void ResetCameraView();

        void FocusCameraView( Entity* pTarget );

        void FocusCameraView( AABB const& worldBounds );

        void FocusCameraView( Vector const& worldPoint, float viewDistance = 1.0f );

        void SetCameraSpeed( float cameraSpeed );

        void SetCameraTransform( Transform const& cameraTransform );

        // This is called when the focus shortcut key is pressed, each editor needs to handle this
        virtual void FocusOnSelected() {}

        Transform GetCameraTransform() const;

    protected:

        // Set the tool tab-title (pass by value is intentional)
        virtual void SetDisplayName( String name );

        // Save
        //-------------------------------------------------------------------------

        // Optional: Override this function to actually save your editor tool data - never call this function directly!
        virtual bool SaveData() { return true; }

        // Docking and Tool Windows
        //-------------------------------------------------------------------------

        // Set up initial docking layout, if the tool has a viewport window, then that viewport is docked to the central node
        virtual void SetupDockingLayout( ImGuiID const dockspaceID, ImVec2 const& dockspaceSize ) const {}

        // Get the tool window class
        ImGuiWindowClass* GetToolWindowClass() { return &m_toolWindowClass; }

        // Create a tool window
        ToolWindow* CreateToolWindow( String const& name, TFunction<void( UpdateContext const&, bool )> const& drawFunction, ImVec2 const& windowPadding = ImVec2( -1, -1 ), bool disableScrolling = false );

        // Get all created tool windows
        EE_FORCE_INLINE TVector<ToolWindow*> const& GetToolWindows() const { return m_toolWindows; }

        // Generate the actual final tool window name within the context of the current tool
        EE_FORCE_INLINE InlineString GetToolWindowName( String const& name ) const { return GetToolWindowName( name.c_str(), m_currentDockspaceID ); }

        // Menu and Help
        //-------------------------------------------------------------------------

        // Call this to draw a help menu row ( label + comes text )
        void DrawHelpTextRow( char const* pLabel, char const* pText ) const;

        // Call this to draw a custom help text row (i.e. not just some text)
        void DrawHelpTextRowCustom( char const* pLabel, TFunction<void()> const& function ) const;

        // Override this function to fill out the help text menu
        virtual void DrawHelpMenu() const { DrawHelpTextRow( "No Help Available", "" ); }

        // Resource Helpers
        //-------------------------------------------------------------------------

        inline FileSystem::Path const& GetRawResourceDirectoryPath() const { return m_pToolsContext->m_pFileRegistry->GetSourceDataDirectoryPath(); }

        inline FileSystem::Path const& GetCompiledResourceDirectoryPath() const { return m_pToolsContext->m_pFileRegistry->GetCompiledResourceDirectoryPath(); }

        inline FileSystem::Path GetFileSystemPath( DataPath const& path ) const
        {
            EE_ASSERT( path.IsValid() );
            return path.GetFileSystemPath( m_pToolsContext->m_pFileRegistry->GetSourceDataDirectoryPath() );
        }

        inline FileSystem::Path GetFileSystemPath( ResourceID const& resourceID ) const
        {
            EE_ASSERT( resourceID.IsValid() );
            return resourceID.GetFileSystemPath( m_pToolsContext->m_pFileRegistry->GetSourceDataDirectoryPath() );
        }

        // Use this function to load a resource required for this tool (hot-reload aware)
        void LoadResource( Resource::ResourcePtr* pResourcePtr );

        // Use this function to unload a required resource for this tool (hot-reload aware)
        void UnloadResource( Resource::ResourcePtr* pResourcePtr );

        // Are we currently loading any resources?
        inline bool IsLoadingResources() const { return !m_loadingResources.empty(); }

        // Are we currently mid hot-reload
        inline bool IsHotReloading() const { return !m_resourcesToBeReloaded.empty(); }

        // Called just before we unload a requested resource
        virtual void OnResourceUnload( Resource::ResourcePtr* pResourcePtr ) {}

        // Called whenever we finish loading a requested resource
        virtual void OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr ) {}

        // Viewport
        //-------------------------------------------------------------------------

        // Get the closest picking ID for this update
        PickingID GetPickingID() const { return m_pickingData.empty() ? PickingID() : m_pickingData.front(); }

        // Get all the picking data for this update
        PickingData const& GetPickingData() const { return m_pickingData; }

        // Called during the viewport drawing allowing the editor tools to handle drag and drop requests
        virtual void OnDragAndDropIntoViewport( Viewport* pViewport ) {}

        // Called whenever we have a valid resource being dropped into the viewport, users can override to provide custom handling
        virtual void DropResourceInViewport( ResourceID const& resourceID, Vector const& worldPosition ) {}

        // Draws the viewport toolbar
        void DrawViewportToolbar( UpdateContext const& context, Viewport* pViewport );

        // Draw the viewport render controls toolbar
        void DrawViewportToolBar_ViewportVisualizationControls( UpdateContext const& context, Viewport* pViewport );

        // Draw the viewport camera controls toolbar
        void DrawViewportToolBar_CameraControls( UpdateContext const& context, Viewport* pViewport );

        // Draw the viewport tool visualization controls toolbar
        void DrawViewportToolBar_VisualizationControls( UpdateContext const& context, Viewport* pViewport );

        // Draw the viewport time controls toolbar
        void DrawViewportToolBar_TimeControls( UpdateContext const& context, Viewport* pViewport );

        // Extend the viewport toolbar by adding custom widgets and controls
        virtual void ExtendViewportToolBar( UpdateContext const& context, Viewport* pViewport ) {}

        // Extend the viewport render controls toolbar, return true if an control was added
        virtual void ExtendViewportToolBar_RenderControls( UpdateContext const& context, Viewport* pViewport ) {}

        // Extend the viewport camera controls toolbar, return true if an control was added
        virtual void ExtendViewportToolBar_CameraControls( UpdateContext const& context, Viewport* pViewport ) {}

        // Extend the viewport visualization controls toolbar, return true if an control was added
        virtual bool ExtendViewportToolBar_VisualizationControls( UpdateContext const& context, Viewport* pViewport ) { return false; }

        // Begin a toolbar group - You need to call end whether or not this returns true (match imgui child window pattern)
        bool BeginViewportToolbarGroup( char const* pGroupID, ImVec2 groupSize, ImVec2 const& padding = ImVec2( 4.0f, 4.0f ) );

        // End a toolbar group, always call this if you called "BeginViewportToolbarGroup"
        void EndViewportToolbarGroup();

        // Creates a viewport drop down 
        void DrawViewportToolbarCombo( char const* pID, char const* pLabel, char const* pTooltip, TFunction<void()> const& function, float width = -1 );

        // Creates a fixed width viewport drop down with an icon as a label
        void DrawViewportToolbarComboIcon( char const* pID, char const* pIcon, char const* pTooltip, TFunction<void()> const& function );

        // Called once per update if we are hovering over the 3D viewport and not over an imgui item
        // Use this to handle any interactions with the 3D world
        virtual void HandleViewportInteractions() {}

        // Editor Map
        //-------------------------------------------------------------------------

        void SpawnEditorMapEntities();

        void DespawnEditorMapEntities();

        void DrawGrid();

        // Entity World Helpers
        //-------------------------------------------------------------------------

        // Helper function for debug drawing
        DebugDrawContext GetDebugDrawContext();

        // Add an entity to the preview world
        // Ownership is transferred to the world, you dont need to call remove/destroy if you dont explicitly need to remove an entity
        void AddEntityToWorld( Entity* pEntity );

        // Removes an entity from the preview world
        // Ownership is transferred back to calling code, so you need to delete it manually
        void RemoveEntityFromWorld( Entity* pEntity );

        // Destroys an entity from the world - pEntity will be set to nullptr
        void DestroyEntityInWorld( Entity*& pEntity );

        // Camera
        //-------------------------------------------------------------------------

        // Create the tool camera
        void CreateCamera();

        // Previewing Support
        //-------------------------------------------------------------------------
        // Provide support to load and preview a specified resource based on the edited data file
        // This is built into the data file editor to support cases where we have a single data file
        // that results in multiple sub-resources

        // Request that we start a preview, this will save the datafile and request load for the preview resource
        void RequestPreview();

        // Stop previewing
        void StopPreview();

        // Are we currently previewing or waiting for a preview to start
        bool IsPreviewing() const { return ( m_previewState == PreviewState::Previewing ); }

        // Are we currently previewing or waiting for a preview to start
        bool IsPreviewingOrWaitingForPreview() const { return ( m_previewState != PreviewState::None ); }

        // Called when the preview resource has been successfully loaded
        virtual void OnPreviewStarted() {}

        // Called when a successfully started preview ends - will only be called if you got a "preview started" call
        virtual void OnPreviewStopped() {}

        // Get the resource ID of the resource we need to load to start the preview
        virtual TInlineVector<ResourceID, 2> GetPreviewResourceIDs() const { EE_UNREACHABLE_CODE(); return {}; }

        // Get the loaded resource (only valid to call when actually previewing or during 'OnPreviewStarted'/'OnPreviewEnded')
        template<typename T>
        T const* GetPreviewResource( ResourceID const& resourceID ) const
        {
            static_assert( std::is_base_of<Resource::IResource, T>::value, "T is not derived from IResource" );
            EE_ASSERT( m_previewState != PreviewState::None && m_previewState != PreviewState::Requested );

            for ( auto& resourcePtr : m_previewResources )
            {
                if ( resourcePtr.GetResourceID() == resourceID )
                {
                    EE_ASSERT( resourcePtr.IsLoaded() );
                    return resourcePtr.GetPtr<T>();
                }
            }

            EE_UNREACHABLE_CODE();
            return nullptr;
        }

        // Get the list of preview resources
        TVector<Resource::ResourcePtr> const& GetPreviewResources() const { return m_previewResources; }

        // Hot-reload
        //-------------------------------------------------------------------------

        // Unload all resources that require hot-reloading
        virtual void HotReload_UnloadResources( TInlineVector<Resource::ResourceRequesterID, 20> const& usersToReload, TInlineVector<ResourceID, 20> const& resourcesToBeReloaded );

        // Try to reload all resources that were hot-reloaded - some editor tools cant function without some data and so if that reload fails then we return true and the the editor will destroy this tool
        virtual bool HotReload_ReloadResources( TInlineVector<Resource::ResourceRequesterID, 20> const& usersToReload, TInlineVector<ResourceID, 20> const& resourcesToBeReloaded );

    private:

        EditorTool& operator=( EditorTool const& ) = delete;
        EditorTool( EditorTool const& ) = delete;

        // Docking And Tool Windows
        //-------------------------------------------------------------------------

        // Calculate the dockspace ID for this tool relative to its parent
        inline ImGuiID CalculateDockspaceID() const
        {
            int32_t dockspaceID = m_currentLocationID;
            char const* const pEditorToolTypeName = GetUniqueTypeName();
            dockspaceID = ImHashData( pEditorToolTypeName, strlen( pEditorToolTypeName ), dockspaceID );
            return dockspaceID;
        }

        // Create a viewport tool window
        ToolWindow* CreateViewportToolWindow( String const& name, Viewport* pViewport, TFunction<void( UpdateContext const&, bool )> const& drawFunction, ImVec2 const& windowPadding = ImVec2( -1, -1 ), bool disableScrolling = false );

        // Draw the viewport for this tool
        void DrawViewportWindow( UpdateContext const& context, Viewport* pViewport );

        // Called by the editor before the main update, this handles a lot of the shared functionality (undo/redo/etc...)
        void PreDrawUpdate( UpdateContext const& context, bool isVisible, bool isFocused );

        // Called by the editor after the main update, this handles a lot of the shared functionality (camera state, etc...)
        void PostDrawUpdate( UpdateContext const& context, bool isVisible, bool isFocused );

        // Menu and Help
        //-------------------------------------------------------------------------

        void DrawMainMenu( UpdateContext const& context );

    protected:

        ToolsContext const*                         m_pToolsContext = nullptr;
        Resource::ResourceSystem*                   m_pResourceSystem = nullptr;
        String                                      m_windowName;
        ImGuiID                                     m_ID = 0; // Document identifier (unique)

        // World
        EntityWorld*                                m_pWorld = nullptr;
        ToolsCameraComponent*                       m_pCamera = nullptr;

        // Viewport
        bool                                        m_isViewportFocused = false;
        bool                                        m_isViewportHovered = false;

        // Undo/Redo
        UndoStack                                   m_undoStack;
        IUndoableAction*                            m_pActiveUndoableAction = nullptr;

    private:

        ImGuiID                                     m_currentDockID = 0;        // The dock we are currently in
        ImGuiID                                     m_desiredDockID = 0;        // The dock we wish to be in
        ImGuiID                                     m_currentLocationID = 0;    // Current Dock node we are docked into _OR_ window ID if floating window
        ImGuiID                                     m_previousLocationID = 0;   // Previous dock node we are docked into _OR_ window ID if floating window
        ImGuiID                                     m_currentDockspaceID = 0;   // Dockspace ID ~~ Hash of LocationID + toolType
        ImGuiID                                     m_previousDockspaceID = 0;
        TVector<ToolWindow*>                        m_toolWindows;
        ImGuiWindowClass                            m_toolWindowClass;  // All our tools windows will share the same WindowClass (based on ID) to avoid mixing tools from different top-level window

        bool                                        m_wasInitializedCalled = false;
        bool                                        m_requiresLayoutReset = false;

        // Viewport
        PickingData                                 m_pickingData;

        // Saving
        bool                                        m_isDirty = false;

        // Resource Management
        TVector<Resource::ResourcePtr*>             m_requestedResources;
        TVector<Resource::ResourcePtr*>             m_loadingResources;
        TVector<Resource::ResourcePtr*>             m_resourcesToBeReloaded;

        // Editor Map
        Entity*                                     m_pEditorMapEntity = nullptr;
        Render::StaticMeshComponent*                m_pFloorMeshComponent = nullptr;
        Physics::CollisionMeshComponent*            m_pFloorCollisionComponent = nullptr;
        Render::StaticMeshComponent*                m_pSkyBoxMeshComponent = nullptr;
        Render::DirectionalLightComponent*          m_pLightComponent = nullptr;

        // Preview
        PreviewState                                m_previewState = PreviewState::None;
        TVector<Resource::ResourcePtr>              m_previewResources;
        CountdownTimer<PlatformClock>               m_previewDelayTimer;
    };

    //-------------------------------------------------------------------------

    namespace ViewportNotificationWindows
    {
        void DrawLoadingWindow();
        void DrawResourceCompilationFailedWindow( String const& compilationLog );
    }
}

#define EE_EDITOR_TOOL( TypeName ) \
constexpr static char const* const s_uniqueTypeName = #TypeName;\
constexpr static uint64_t const s_toolTypeID = Hash::FNV1a::GetHash64( #TypeName );\
constexpr static bool const s_isSingleton = false; \
virtual char const* GetUniqueTypeName() const override { return s_uniqueTypeName; }\
virtual uint64_t GetUniqueTypeID() const override final { return TypeName::s_toolTypeID; }

//-------------------------------------------------------------------------

#define EE_SINGLETON_EDITOR_TOOL( TypeName ) \
constexpr static char const* const s_uniqueTypeName = #TypeName;\
constexpr static uint64_t const s_toolTypeID = Hash::FNV1a::GetHash64( #TypeName ); \
constexpr static bool const s_isSingleton = true; \
virtual char const* GetUniqueTypeName() const { return s_uniqueTypeName; }\
virtual uint64_t GetUniqueTypeID() const override final { return TypeName::s_toolTypeID; }\
virtual bool IsSingleton() const override final { return true; }

//-------------------------------------------------------------------------
// Data File Editor
//-------------------------------------------------------------------------

namespace EE
{
    class DataFileEditor : public EditorTool
    {
        friend class ScopedDataFileModification;
        friend class DataFileUndoableAction;
        friend class EditorUI;

    public:

        explicit DataFileEditor( ToolsContext const* pToolsContext, DataPath const& dataPath, EntityWorld* pWorld = nullptr );
        virtual ~DataFileEditor();

        virtual bool IsDataFileTool() const override final { return true; }
        virtual bool IsEditingResourceDescriptor() const;
        virtual bool IsEditingFile( DataPath const& dataPath ) const override { return m_dataFilePath == dataPath; }
        virtual bool HasDependencyOnFile( DataPath const& dataPath ) const override { return m_dataFilePath == dataPath; }

    protected:

        // Editor Tool
        //-------------------------------------------------------------------------

        virtual InlineString GetName() const override { return InlineString( GetUniqueTypeName() ) + " (" + GetDataFilePath().c_str() + ")"; }

        // Note: We try to load the data file during initialize, derived classes will have access to the data file in their initialize if the load was successful
        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;

        virtual void DrawMenu( UpdateContext const& context ) override;

        virtual void MarkDirty() override;
        virtual void ClearDirty() override;

        virtual bool SupportsSaving() const override final { return true; }
        virtual String GetFilenameForSave() const override;
        virtual bool SaveData() override;

        virtual void PreUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) override;
        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) override;

        virtual void HotReload_UnloadResources( TInlineVector<Resource::ResourceRequesterID, 20> const& usersToReload, TInlineVector<ResourceID, 20> const& resourcesToBeReloaded ) final;
        virtual bool HotReload_ReloadResources( TInlineVector<Resource::ResourceRequesterID, 20> const& usersToReload, TInlineVector<ResourceID, 20> const& resourcesToBeReloaded ) final;

        // Data File
        //-------------------------------------------------------------------------

        // Has the data file been loaded (only valid for resource editors)
        inline bool IsDataFileLoaded() const { return m_pDataFile != nullptr; }

        // Get the path for the data file we're working on
        DataPath const& GetDataFilePath() const { return m_dataFilePath; }

        // Get the file path for the data file we're working on
        FileSystem::Path GetDataFileSystemPath() const { return GetFileSystemPath( m_dataFilePath ); }

        // Get the data file as a derived type (Note! You need to check if the data file is actually loaded)
        template<typename T>
        T* GetDataFile() { return Cast<T>( m_pDataFile ); }

        // Get the data file as a derived type (Note! You need to check if the data file is actually loaded)
        template<typename T>
        T const* GetDataFile() const { return Cast<T>( m_pDataFile ); }

        // Call this immediately before you change the data file data (this serializes the original state of the data file state into the undo-redo-stack)
        void BeginDataFileModification();

        // Call this once you are finished modifying the data file state (this serializes the modified state of the data file into the undo-redo-stack)
        void EndDataFileModification();

        // Draws a separate data file property grid editor window - return true if focused
        void DrawDataFileEditorWindow( UpdateContext const& context, bool isFocused );

        // Unloads the specified data file in the data file ID member
        void UnloadDataFile();

        // Called just before we unload the tool data file
        virtual void OnDataFileUnload() {}

        // Loads the specified data file in the data file ID member
        void LoadDataFile();

        // Called whenever we finish loading the tool data file
        virtual void OnDataFileLoadCompleted() {}

        // Are we allowed to edit the data file directly?
        virtual bool IsDataFileManualEditingAllowed() const { return true; }

        // Show the data file Window
        void ShowDataFileWindow();

        // Hide the data file Window
        void HideDataFileWindow();

    protected:

        PropertyGrid*                               m_pDataFilePropertyGrid = nullptr;
        PropertyGrid::VisualState                   m_dataFilePropertyGridVisualState;

    private:

        int32_t                                     m_beginDataFileModificationCallCount = 0;
        DataPath                                    m_dataFilePath;
        IDataFile*                                  m_pDataFile = nullptr;
        EventBindingID                              m_preDataFileEditEventBindingID;
        EventBindingID                              m_postDataFileEditEventBindingID;
    };

    //-------------------------------------------------------------------------

    class DataFileUndoableAction final : public IUndoableAction
    {
        EE_REFLECT_TYPE( DataFileUndoableAction );

    public:

        DataFileUndoableAction() = default;
        DataFileUndoableAction( TypeSystem::TypeRegistry const* pTypeRegistry, DataFileEditor* pEditor );

        virtual void Undo() override;
        virtual void Redo() override;
        void SerializeBeforeState();
        void SerializeAfterState();

    private:

        TypeSystem::TypeRegistry const*     m_pTypeRegistry = nullptr;
        DataFileEditor*                     m_pEditor = nullptr;
        String                              m_valueBefore;
        String                              m_valueAfter;
        Log                                 m_log;
    };

    //-------------------------------------------------------------------------

    class [[nodiscard]] ScopedDataFileModification
    {
    public:

        ScopedDataFileModification( DataFileEditor* pEditor ) : m_pEditor( pEditor ) { EE_ASSERT( pEditor != nullptr ); m_pEditor->BeginDataFileModification(); }
        virtual ~ScopedDataFileModification() { m_pEditor->EndDataFileModification(); }

    private:

        DataFileEditor*  m_pEditor = nullptr;
    };
}

//-------------------------------------------------------------------------
// Resource Editor
//-------------------------------------------------------------------------

namespace EE
{
    template<typename T>
    class TResourceEditor : public DataFileEditor
    {
        static_assert( std::is_base_of<Resource::IResource, T>::value, "T must derived from IResource" );

    public:

        TResourceEditor( ToolsContext const* pToolsContext, ResourceID const& resourceID, EntityWorld* pWorld = nullptr )
            : DataFileEditor( pToolsContext, resourceID.GetDataPath(), pWorld )
            , m_editedResource( resourceID )
        {
            EE_ASSERT( resourceID.IsValid() );
        }

        virtual void Initialize( UpdateContext const& context ) override
        {
            DataFileEditor::Initialize( context );

            LoadResource( &m_editedResource );
        }

        virtual void Shutdown( UpdateContext const& context ) override
        {
            if ( m_editedResource.WasRequested() )
            {
                UnloadResource( &m_editedResource );
            }

            DataFileEditor::Shutdown( context );
        }

        virtual bool IsEditingResourceDescriptor() const override final { return true; }

        virtual bool SupportsNewFileCreation() const { return GetDataFile<Resource::ResourceDescriptor>()->IsUserCreateableDescriptor(); }

        virtual void CreateNewFile() const override { m_pToolsContext->TryCreateNewResourceDescriptorOrDataFile( GetDataFile<Resource::ResourceDescriptor>()->GetTypeID() ); }

        // Resource Status
        //-------------------------------------------------------------------------

        inline bool IsLoading() const { return m_editedResource.IsLoading(); }
        inline bool IsUnloaded() const { return m_editedResource.IsUnloaded(); }
        inline bool IsResourceLoaded() const { return m_editedResource.IsLoaded(); }
        inline bool IsWaitingForResource() const { return IsLoading() || IsUnloaded() || m_editedResource.IsUnloading(); }
        inline bool HasLoadingFailed() const { return m_editedResource.HasLoadingFailed(); }

    protected:

        virtual void DrawViewportUI( UpdateContext const& context, Viewport const* pViewport, bool isFocused ) override
        {
            DataFileEditor::DrawViewportUI( context, pViewport, isFocused );

            //-------------------------------------------------------------------------

            if ( m_editedResource.HasLoadingFailed() )
            {
                ViewportNotificationWindows::DrawResourceCompilationFailedWindow( m_editedResource.GetResourceCompilationLog() );
            }
        }

        virtual TInlineVector<ResourceID, 2> GetPreviewResourceIDs() const final { return { m_editedResource.GetResourceID() }; }

        // Get the loaded resource (only valid to call when actually previewing or during 'OnPreviewStarted'/'OnPreviewEnded')
        template<typename PRT>
        PRT const* GetPreviewResource() const { return DataFileEditor::GetPreviewResource<PRT>( m_editedResource.GetResourceID() ); }

    protected:

        TResourcePtr<T>                             m_editedResource;
    };
}

//-------------------------------------------------------------------------
// Editor Tool Factory
//-------------------------------------------------------------------------

namespace EE
{
    class EE_ENGINETOOLS_API EditorToolFactory : public TGlobalRegistryBase<EditorToolFactory>
    {
        EE_GLOBAL_REGISTRY( EditorToolFactory );

    public:

        virtual ~EditorToolFactory() = default;

        static EditorTool* TryCreateEditor( ToolsContext const* pToolsContext, DataPath const& path, TFunction< EntityWorld*( )> worldCreationFunction );

    protected:

        // Is this a resource editor factory?
        virtual bool IsResourceEditorFactory() const { return false; }

        // Get the resource type this factory supports
        virtual ResourceTypeID GetSupportedResourceTypeID() const { return ResourceTypeID(); }

        // Get the resource type this factory supports
        virtual DataFileExtension GetSupportedDataFileExtension() const { return 0; }

        // Do we need an editor world?
        virtual bool RequiresEntityWorld() const = 0;

        // Virtual method that will create a tool if the resource ID matches the appropriate types
        virtual EditorTool* CreateEditorInternal( ToolsContext const* pToolsContext, ResourceID const& resourceID, EntityWorld* pWorld = nullptr ) const = 0;
    };
}

//-------------------------------------------------------------------------
//  Macro to create a editor tool factories
//-------------------------------------------------------------------------
// Use in a CPP to define a factory:
// * EE_DATAFILE_EDITOR_FACTORY_NO_WORLD( PhysicMaterialsEditorFactory, "pml", SkeletonEditor );
// * EE_RESOURCE_EDITOR_FACTORY( SkeletonEditorFactory, Skeleton, SkeletonEditor );

//-------------------------------------------------------------------------

#define EE_DATAFILE_EDITOR_FACTORY( factoryName, extension, editorClass )\
class factoryName final : public EditorToolFactory\
{\
    virtual DataFileExtension GetSupportedDataFileExtension() const override { EE_ASSERT( FourCC::IsValidLowercase( extensionFourCC ) ); return extensionFourCC; }\
    virtual bool RequiresEntityWorld() const override { return true; }\
    virtual EditorTool* CreateEditorInternal( ToolsContext const* pToolsContext, DataPath const& path, EntityWorld* pWorld ) const override\
    {\
        EE_ASSERT( pWorld != nullptr ); \
        return EE::New<editorClass>( pToolsContext, path, pWorld );\
    }\
};\
static factoryName g_##factoryName;

//-------------------------------------------------------------------------

#define EE_DATAFILE_EDITOR_FACTORY_NO_WORLD( factoryName, extension, editorClass )\
class factoryName final : public EditorToolFactory\
{\
    virtual DataFileExtension GetSupportedDataFileExtension() const override { EE_ASSERT( FourCC::IsValidLowercase( extensionFourCC ) ); return extensionFourCC; }\
    virtual bool RequiresEntityWorld() const override { return false; }\
    virtual EditorTool* CreateEditorInternal( ToolsContext const* pToolsContext, DataPath const& path, EntityWorld* pWorld ) const override\
    {\
        EE_ASSERT( pWorld == nullptr );\
        return EE::New<editorClass>( pToolsContext, path );\
    }\
};\
static factoryName g_##factoryName;

//-------------------------------------------------------------------------

#define EE_RESOURCE_EDITOR_FACTORY( factoryName, resourceClass, editorClass )\
class factoryName final : public EditorToolFactory\
{\
    virtual bool IsResourceEditorFactory() const override { return true; }\
    virtual ResourceTypeID GetSupportedResourceTypeID() const override { return resourceClass::GetStaticResourceTypeID(); }\
    virtual bool RequiresEntityWorld() const override { return true; }\
    virtual EditorTool* CreateEditorInternal( ToolsContext const* pToolsContext, ResourceID const& resourceID, EntityWorld* pWorld ) const override\
    {\
        EE_ASSERT( pWorld != nullptr ); \
        EE_ASSERT( resourceID.GetResourceTypeID() == resourceClass::GetStaticResourceTypeID() );\
        return EE::New<editorClass>( pToolsContext, resourceID, pWorld );\
    }\
};\
static factoryName g_##factoryName;

//-------------------------------------------------------------------------

#define EE_RESOURCE_EDITOR_FACTORY_NO_WORLD( factoryName, resourceClass, editorClass )\
class factoryName final : public EditorToolFactory\
{\
    virtual bool IsResourceEditorFactory() const override { return true; }\
    virtual ResourceTypeID GetSupportedResourceTypeID() const override { return resourceClass::GetStaticResourceTypeID(); }\
    virtual bool RequiresEntityWorld() const override { return false; }\
    virtual EditorTool* CreateEditorInternal( ToolsContext const* pToolsContext, ResourceID const& resourceID, EntityWorld* pWorld ) const override\
    {\
        EE_ASSERT( pWorld == nullptr );\
        EE_ASSERT( resourceID.GetResourceTypeID() == resourceClass::GetStaticResourceTypeID() );\
        return EE::New<editorClass>( pToolsContext, resourceID );\
    }\
};\
static factoryName g_##factoryName;
