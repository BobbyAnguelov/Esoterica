#pragma once
#include "UndoStack.h"
#include "ToolsContext.h"
#include "DialogManager.h"
#include "PropertyGrid/PropertyGrid.h"
#include "EngineTools/Resource/ResourceDatabase.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Imgui/ImguiGizmo.h"
#include "Base/Utils/GlobalRegistryBase.h"
#include "Base/Serialization/JsonSerialization.h"
#include "Base/Resource/ResourcePtr.h"

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
        friend class ScopedDescriptorModification;
        friend class ResourceDescriptorUndoableAction;

    public:

        constexpr static char const* const s_descriptorWindowName = "Descriptor";
        constexpr static char const* const s_viewportWindowName = "Viewport";

        class ToolWindow
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

            inline bool HasUserSpecifiedWindowPadding() const
            {
                return m_windowPadding.x >= 0 && m_windowPadding.y >= 0;
            }

            inline bool IsScrollingDisabled() const
            {
                return m_disableScrolling;
            }

        protected:

            String                                          m_name;
            TFunction<void( UpdateContext const&, bool )>   m_drawFunction;
            ImVec2                                          m_windowPadding;
            bool                                            m_disableScrolling = false;
            bool                                            m_isViewport = false;
            bool                                            m_isOpen = true;
        };

        struct ViewportInfo
        {
            ImTextureID                                     m_pViewportRenderTargetTexture = nullptr;
            TFunction<Render::PickingID( Int2 const& )>     m_retrievePickingID;
        };

    protected:

        // Create the tool window name
        EE_FORCE_INLINE static InlineString GetToolWindowName( char const* pToolWindowName, ImGuiID dockspaceID )
        {
            EE_ASSERT( pToolWindowName != nullptr );
            return InlineString( InlineString::CtorSprintf(), "%s##%08X", pToolWindowName, dockspaceID );
        }

    public:

        explicit EditorTool( ToolsContext const* pToolsContext, String const& displayName );
        explicit EditorTool( ToolsContext const* pToolsContext, String const& displayName, EntityWorld* pWorld );
        explicit EditorTool( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID );

        virtual ~EditorTool();

        // Info
        //-------------------------------------------------------------------------

        // Get the hash of the unique type ID for this tool
        virtual uint32_t GetUniqueTypeID() const = 0;

        // Get the unique typename for this tool to be used for docking
        virtual char const* GetUniqueTypeName() const = 0;

        // Was the initialization function called
        inline bool IsInitialized() const { return m_isInitialized; }

        // Are we operating on a resource descriptor?
        inline bool IsResourceEditor() const { return m_descriptorID.IsValid(); }

        // Is this tool currently working on this resource. This is necessary since we have situations where multiple resourceIDs all map to the same tool.
        // For example, all graph variations should open the tool for the parent graph.
        virtual bool IsEditingResource( ResourceID const& resourceID ) const { return resourceID == m_descriptorID; }

        // Is this resource necessary for this tool to function. Primarily needed to determine when to automatically close resource editors due to external resource deletion.
        // Again we need potential derivations due to cases where multiple resources tie into a single tool.
        virtual bool HasDependencyOnResource( ResourceID const& resourceID ) const { return resourceID == m_descriptorID; }

        // Is Singleton tool - i.e. is only one instance of this tool allowed
        virtual bool IsSingleton() const { return false; }

        // Is this tool a single window tool? i.e. no dockspace, only one tool window, etc...
        virtual bool IsSingleWindowTool() const { return false; }

        // Get the display name for this tool (shown on tab, dialogs, etc...)
        inline char const* GetDisplayName() const { return m_windowName.c_str(); }

        // Does this tool have a title bar icon
        virtual bool HasTitlebarIcon() const { return false; }

        // Get this tool's title bar icon
        virtual char const* GetTitlebarIcon() const { EE_ASSERT( HasTitlebarIcon() ); return nullptr; }

        // Docking and Tool Windows
        //-------------------------------------------------------------------------

        // Set up initial docking layout
        virtual void InitializeDockingLayout( ImGuiID const dockspaceID, ImVec2 const& dockspaceSize ) const;

        // Lifetime/Update Functions
        //-------------------------------------------------------------------------

        // Initialize the tool: initialize window IDs, load data, etc...
        // NOTE: Derived classes MUST call base implementation!
        virtual void Initialize( UpdateContext const& context );

        // Shutdown the tool, free allocated memory, etc...
        virtual void Shutdown( UpdateContext const& context );

        // Called just before the world is updated for each update stage
        virtual void PreWorldUpdate( EntityWorldUpdateContext const& updateContext ) {}

        // Frame update and draw any tool windows needed for the tool
        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) {}

        // Called by the editor before the main update, this handles a lot of the shared functionality (undo/redo/etc...)
        virtual void SharedUpdate( UpdateContext const& context, bool isVisible, bool isFocused );

        // Menu and Help
        //-------------------------------------------------------------------------

        // Does this tool have a toolbar?
        virtual bool SupportsMainMenu() const { return true; }

        // Draws the tool toolbar menu
        virtual void DrawMenu( UpdateContext const& context ) {}

        // Undo/Redo
        //-------------------------------------------------------------------------

        // Does this tool support saving
        virtual bool SupportsUndoRedo() const { return true; }

        // Called immediately before we execute an undo or redo action - undo/redo commands occur before the tool "update" call
        virtual void PreUndoRedo( UndoStack::Operation operation ) {}

        // Called immediately after we execute an undo or redo action - undo/redo commands occur before the tool "update" call
        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction );

        inline bool CanUndo() { return m_undoStack.CanUndo(); }
        void Undo();

        inline bool CanRedo() { return m_undoStack.CanRedo(); }
        void Redo();

        // Saving and Dirty state
        //-------------------------------------------------------------------------

        // Does this tool support saving
        virtual bool SupportsSaving() const { return IsResourceEditor(); }

        // Flag this tool as dirty
        void MarkDirty() { m_isDirty = true; }

        // Clear the dirty flag
        void ClearDirty() { m_isDirty = false; }

        // Has any modifications been made to this file?
        bool IsDirty() const { return m_isDirty; }

        // Should we always allow saving?
        virtual bool AlwaysAllowSaving() const { return false; }

        // Get the target filename for the save operation
        String GetFilenameForSave() const;

        // Call this function to save! Returns true if successful
        bool Save();

        // Entity World
        //-------------------------------------------------------------------------

        // Should we load the default editor map for this tool?
        virtual bool ShouldLoadDefaultEditorMap() const { return true; }

        // Does this tool have a preview world
        inline bool HasEntityWorld() const { return m_pWorld != nullptr; }

        // Get the world associated with this tool
        inline EntityWorld* GetEntityWorld() const { return m_pWorld; }

        void SetWorldPaused( bool isPaused );

        void SetWorldTimeScale( float newTimeScale );

        void ResetWorldTimeScale();

        // Viewport
        //-------------------------------------------------------------------------

        // Should this tool display a viewport?
        virtual bool SupportsViewport() const { return m_pWorld != nullptr; }

        // Should we draw the time control widget in the viewport toolbar
        virtual bool HasViewportToolbarTimeControls() const { return false; }

        // Does this tool's viewport have a orientation guide drawn?
        virtual bool HasViewportOrientationGuide() const { return true; }

        // A large viewport overlay window is created for all editor tools allowing you to draw helpers and widgets over a viewport
        // Called as part of the tool drawing so at "Frame Start" - so be careful of any timing issue with 3D drawing
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport ) {}

        // Draw the viewport for this tool - returns true if this viewport is currently focused
        virtual void DrawViewport( UpdateContext const& context, ViewportInfo const& viewportInfo );

        // Camera
        //-------------------------------------------------------------------------

        void SetCameraUpdateEnabled( bool isEnabled );

        void ResetCameraView();

        void FocusCameraView( Entity* pTarget );

        void SetCameraSpeed( float cameraSpeed );

        void SetCameraTransform( Transform const& cameraTransform );

        Transform GetCameraTransform() const;

    protected:

        // Set the tool tab-title (pass by value is intentional)
        virtual void SetDisplayName( String name );

        // Save
        //-------------------------------------------------------------------------

        // Optional: Override this function to actually save your editor tool data - never call this function directly!
        virtual bool SaveData();

        // Docking and Tool Windows
        //-------------------------------------------------------------------------

        ImGuiWindowClass* GetToolWindowClass() { return &m_toolWindowClass; }

        void CreateToolWindow( String const& name, TFunction<void( UpdateContext const&, bool )> const& drawFunction, ImVec2 const& windowPadding = ImVec2( -1, -1 ), bool disableScrolling = false );

        EE_FORCE_INLINE TVector<ToolWindow> const& GetToolWindows() const { return m_toolWindows; }

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

        // Use this function to load a resource required for this tool (hot-reload aware)
        void LoadResource( Resource::ResourcePtr* pResourcePtr );

        // Use this function to unload a required resource for this tool (hot-reload aware)
        void UnloadResource( Resource::ResourcePtr* pResourcePtr );

        // Blocking call to force a recompile of a resource outside of the regular resource server flow, this is needed when you need to ensure a resource has been recompiled in advance of loading
        // Will also flag the resource for hot-reload if there are any current users
        bool RequestImmediateResourceCompilation( ResourceID const& resourceID );

        // Called just before we unload a requested resource
        virtual void OnResourceUnload( Resource::ResourcePtr* pResourcePtr ) {}

        // Called whenever we finish loading a requested resource
        virtual void OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr ) {}

        // Viewport
        //-------------------------------------------------------------------------

        // Called whenever we click inside a tool viewport and get a non-zero picking ID
        virtual void OnMousePick( Render::PickingID pickingID ) {}

        // Called during the viewport drawing allowing the editor tools to handle drag and drop requests
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

        // Entity World Helpers
        //-------------------------------------------------------------------------

        // Helper function for debug drawing
        Drawing::DrawContext GetDrawingContext();

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

        // Descriptor
        //-------------------------------------------------------------------------

        // Has the descriptor been loaded (only valid for resource editors)
        inline bool IsDescriptorLoaded() const { return m_pDescriptor != nullptr; }

        // Get the ID for the descriptor we're working on
        ResourceID const& GetDescriptorID() const { EE_ASSERT( IsResourceEditor() ); return m_descriptorID; }

        // Get the filepath for the descriptor we're working on
        FileSystem::Path const& GetDescriptorPath() const { EE_ASSERT( IsResourceEditor() ); return m_descriptorPath; }

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

        // Unloads the specified descriptor in the descriptor ID member
        void UnloadDescriptor();

        // Called just before we unload the tool descriptor
        virtual void OnDescriptorUnload() {}

        // Loads the specified descriptor in the descriptor ID member
        void LoadDescriptor();

        // Called whenever we finish loading the tool descriptor
        virtual void OnDescriptorLoadCompleted() {}

        // Are we allowed to edit the descriptor directly?
        virtual bool IsDescriptorManualEditingAllowed() const { return true; }

        // Show the descriptor Window
        void ShowDescriptorWindow();

        // Hide the descriptor Window
        void HideDescriptorWindow();

    private:

        EditorTool& operator=( EditorTool const& ) = delete;
        EditorTool( EditorTool const& ) = delete;

        // Docking And Tool Windows
        //-------------------------------------------------------------------------

        // Calculate the dockspace ID for this tool relative to its parent
        inline ImGuiID CalculateDockspaceID()
        {
            int32_t dockspaceID = m_currentLocationID;
            char const* const pEditorToolTypeName = GetUniqueTypeName();
            dockspaceID = ImHashData( pEditorToolTypeName, strlen( pEditorToolTypeName ), dockspaceID );
            return dockspaceID;
        }

        // Menu and Help
        //-------------------------------------------------------------------------

        void DrawMainMenu( UpdateContext const& context );

        // Hot-reload
        //-------------------------------------------------------------------------

        virtual void HotReload_UnloadResources( TVector<Resource::ResourceRequesterID> const& usersToBeReloaded, TVector<ResourceID> const& resourcesToBeReloaded );
        virtual void HotReload_ReloadResources();

    protected:

        ToolsContext const*                         m_pToolsContext = nullptr;
        Resource::ResourceSystem*                   m_pResourceSystem = nullptr;
        String                                      m_windowName;
        ImGuiID                                     m_ID = 0; // Document identifier (unique)
        DialogManager                               m_dialogManager;

        // World
        EntityWorld*                                m_pWorld = nullptr;
        DebugCameraComponent*                       m_pCamera = nullptr;
        float                                       m_worldTimeScale = 1.0f;

        // Viewport
        bool                                        m_isViewportFocused = false;
        bool                                        m_isViewportHovered = false;

        // Undo/Redo
        UndoStack                                   m_undoStack;
        IUndoableAction*                            m_pActiveUndoableAction = nullptr;

        // Descriptor
        PropertyGrid*                               m_pDescriptorPropertyGrid = nullptr;

    private:

        ImGuiID                                     m_currentDockID = 0;        // The dock we are currently in
        ImGuiID                                     m_desiredDockID = 0;        // The dock we wish to be in
        ImGuiID                                     m_currentLocationID = 0;    // Current Dock node we are docked into _OR_ window ID if floating window
        ImGuiID                                     m_previousLocationID = 0;   // Previous dock node we are docked into _OR_ window ID if floating window
        ImGuiID                                     m_currentDockspaceID = 0;   // Dockspace ID ~~ Hash of LocationID + toolType
        ImGuiID                                     m_previousDockspaceID = 0;
        TVector<ToolWindow>                         m_toolWindows;
        ImGuiWindowClass                            m_toolWindowClass;  // All our tools windows will share the same WindowClass (based on ID) to avoid mixing tools from different top-level window

        bool                                        m_isInitialized = false;

        // Undo/Redo
        int32_t                                     m_beginModificationCallCount = 0;

        // Saving
        bool                                        m_isDirty = false;
        bool                                        m_isSavingAllowedValidation = false;

        // Descriptor
        ResourceID                                  m_descriptorID;
        FileSystem::Path                            m_descriptorPath;
        Resource::ResourceDescriptor*               m_pDescriptor = nullptr;
        EventBindingID                              m_preEditEventBindingID;
        EventBindingID                              m_postEditEventBindingID;

        // Resource Management
        TVector<Resource::ResourcePtr*>             m_requestedResources;
        TVector<Resource::ResourcePtr*>             m_loadingResources;
        TVector<Resource::ResourcePtr*>             m_resourcesToBeReloaded;
    };

    //-------------------------------------------------------------------------

    template<typename T>
    class TResourceEditor : public EditorTool
    {
        static_assert( std::is_base_of<Resource::IResource, T>::value, "T must derived from IResource" );

    public:

        // Specify whether to initially load the resource, this is not necessary for all editors
        TResourceEditor( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID )
            : EditorTool( pToolsContext, pWorld, resourceID )
            , m_editedResource( resourceID )
        {
            EE_ASSERT( resourceID.IsValid() );
        }

        virtual void Initialize( UpdateContext const& context ) override
        {
            EditorTool::Initialize( context );

            LoadResource( &m_editedResource );
        }

        virtual void Shutdown( UpdateContext const& context ) override
        {
            if ( m_editedResource.WasRequested() )
            {
                UnloadResource( &m_editedResource );
            }

            EditorTool::Shutdown( context );
        }

        // Resource Status
        //-------------------------------------------------------------------------

        inline bool IsLoading() const { return m_editedResource.IsLoading(); }
        inline bool IsUnloaded() const { return m_editedResource.IsUnloaded(); }
        inline bool IsResourceLoaded() const { return m_editedResource.IsLoaded(); }
        inline bool IsWaitingForResource() const { return IsLoading() || IsUnloaded() || m_editedResource.IsUnloading(); }
        inline bool HasLoadingFailed() const { return m_editedResource.HasLoadingFailed(); }

    private:

        virtual void DrawViewport( UpdateContext const& context, ViewportInfo const& viewportInfo ) override final
        {
            EditorTool::DrawViewport( context, viewportInfo );

            if ( m_editedResource.HasLoadingFailed() )
            {
                ImGuiX::ScopedFont const sf( ImGuiX::Font::LargeBold );
                ImGui::Indent( 15.0f );
                ImGui::TextColored( Colors::Red.ToFloat4(), "Resource Load Failed:\n%s", m_editedResource.GetResourceCompilationLog().c_str() );
                ImGui::Unindent();
            }
        }

    protected:

        TResourcePtr<T>                     m_editedResource;
    };
}

namespace EE
{
    //-------------------------------------------------------------------------
    // Resource Editor Factory
    //-------------------------------------------------------------------------
    // Used to spawn the appropriate resource editor

    class EE_ENGINETOOLS_API ResourceEditorFactory : public TGlobalRegistryBase<ResourceEditorFactory>
    {
        EE_DECLARE_GLOBAL_REGISTRY( ResourceEditorFactory );

    public:

        virtual ~ResourceEditorFactory() = default;

        static bool CanCreateEditor( ToolsContext const* pToolsContext, ResourceID const& resourceID );
        static EditorTool* CreateEditor( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID );

    protected:

        // Get the resource type this factory supports
        virtual ResourceTypeID GetSupportedResourceTypeID() const { return ResourceTypeID(); }

        // Virtual method that will create a tool if the resource ID matches the appropriate types
        virtual EditorTool* CreateEditorInternal( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID ) const = 0;
    };

    //-------------------------------------------------------------------------
    // Descriptor Undo Action
    //-------------------------------------------------------------------------

    class ResourceDescriptorUndoableAction final : public IUndoableAction
    {
        EE_REFLECT_TYPE( IUndoableAction );

    public:

        ResourceDescriptorUndoableAction() = default;
        ResourceDescriptorUndoableAction( TypeSystem::TypeRegistry const* pTypeRegistry, EditorTool* pEditorTool );

        virtual void Undo() override;
        virtual void Redo() override;
        void SerializeBeforeState();
        void SerializeAfterState();

    private:

        TypeSystem::TypeRegistry const*     m_pTypeRegistry = nullptr;
        EditorTool*                         m_pEditorTool = nullptr;
        String                              m_valueBefore;
        String                              m_valueAfter;
    };

    //-------------------------------------------------------------------------

    class [[nodiscard]] ScopedDescriptorModification
    {
    public:

        ScopedDescriptorModification( EditorTool* pEditorTool ) : m_pEditorTool( pEditorTool ) { EE_ASSERT( pEditorTool != nullptr ); m_pEditorTool->BeginDescriptorModification(); }
        virtual ~ScopedDescriptorModification() { m_pEditorTool->EndDescriptorModification(); }

    private:

        EditorTool*  m_pEditorTool = nullptr;
    };
}

//-------------------------------------------------------------------------

#define EE_EDITOR_TOOL( TypeName ) \
constexpr static char const* const s_uniqueTypeName = #TypeName;\
constexpr static uint32_t const s_toolTypeID = Hash::FNV1a::GetHash32( #TypeName );\
constexpr static bool const s_isSingleton = false; \
virtual char const* GetUniqueTypeName() const override { return s_uniqueTypeName; }\
virtual uint32_t GetUniqueTypeID() const override final { return TypeName::s_toolTypeID; }

//-------------------------------------------------------------------------

#define EE_SINGLETON_EDITOR_TOOL( TypeName ) \
constexpr static char const* const s_uniqueTypeName = #TypeName;\
constexpr static uint32_t const s_toolTypeID = Hash::FNV1a::GetHash32( #TypeName ); \
constexpr static bool const s_isSingleton = true; \
virtual char const* GetUniqueTypeName() const { return s_uniqueTypeName; }\
virtual uint32_t GetUniqueTypeID() const override final { return TypeName::s_toolTypeID; }\
virtual bool IsSingleton() const override final { return true; }

//-------------------------------------------------------------------------
//  Macro to create a resource editor factory
//-------------------------------------------------------------------------
// Use in a CPP to define a factory e.g., EE_RESOURCE_EDITOR_FACTORY( SkeletonEditorFactory, Skeleton, SkeletonEditor );

#define EE_RESOURCE_EDITOR_FACTORY( factoryName, resourceClass, editorClass )\
class factoryName final : public ResourceEditorFactory\
{\
    virtual ResourceTypeID GetSupportedResourceTypeID() const override { return resourceClass::GetStaticResourceTypeID(); }\
    virtual EditorTool* CreateEditorInternal( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID ) const override\
    {\
        EE_ASSERT( resourceID.GetResourceTypeID() == resourceClass::GetStaticResourceTypeID() );\
        return EE::New<editorClass>( pToolsContext, pWorld, resourceID );\
    }\
};\
static factoryName g_##factoryName;