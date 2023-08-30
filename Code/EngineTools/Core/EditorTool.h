#pragma once
#include "UndoStack.h"
#include "ToolsContext.h"
#include "DialogManager.h"
#include "PropertyGrid/PropertyGrid.h"
#include "EngineTools/Resource/ResourceDatabase.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Imgui/ImguiGizmo.h"
#include "Base/Utils/GlobalRegistryBase.h"
#include "Base/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE
{
    class UpdateContext;
}

//-------------------------------------------------------------------------
// Editor Tool
//-------------------------------------------------------------------------
// An editor is a set of tools used to provide information or access to engine feature
// Has no entity world or read/write/load functionality

namespace EE
{
    class EE_ENGINETOOLS_API EditorTool
    {
        friend class EditorUI;
        friend class Workspace;

    public:

        class ToolWindow
        {
            friend class EditorTool;
            friend class EditorUI;
            friend class Workspace;

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
            uint64_t                                        m_userData = 0;
            TFunction<void(UpdateContext const&, bool)>     m_drawFunction;
            ImVec2                                          m_windowPadding;
            bool                                            m_disableScrolling = false;
            bool                                            m_isViewport = false;
            bool                                            m_isOpen = true;
        };

    protected:

        // Create the tool window name
        EE_FORCE_INLINE static InlineString GetToolWindowName( char const* pToolWindowName, ImGuiID dockspaceID )
        {
            EE_ASSERT( pToolWindowName != nullptr );
            return InlineString( InlineString::CtorSprintf(), "%s##%08X", pToolWindowName, dockspaceID );
        }

    public:

        virtual ~EditorTool();

        // Editor Tool
        //-------------------------------------------------------------------------

        virtual uint32_t GetToolTypeID() const = 0;

        // Is Singleton tool
        virtual bool IsSingleton() const { return false; }

        // Is this tool a single window tool? i.e. no dockspace, only one tool window, etc...
        virtual bool IsSingleWindowTool() const { return false; }

        // Get the display name for this tool (shown on tab, dialogs, etc...)
        inline char const* GetDisplayName() const { return m_windowName.c_str(); }

        // Does this tool have a title bar icon
        virtual bool HasTitlebarIcon() const { return false; }

        // Get this tool's title bar icon
        virtual char const* GetTitlebarIcon() const { EE_ASSERT( HasTitlebarIcon() ); return nullptr; }

        // Does this tool have a toolbar?
        virtual bool HasMenu() const { return !IsSingleWindowTool(); }

        // Draws the tool toolbar menu
        virtual void DrawMenu( UpdateContext const& context ) {}

        // Was the initialization function called
        inline bool IsInitialized() const { return m_isInitialized; }

        // Docking
        //-------------------------------------------------------------------------

        // Get the unique typename for this tool to be used for docking
        virtual char const* GetDockingUniqueTypeName() const = 0;

        // Set up initial docking layout
        virtual void InitializeDockingLayout( ImGuiID const dockspaceID, ImVec2 const& dockspaceSize ) const;

        // Lifetime/Update Functions
        //-------------------------------------------------------------------------

        // Initialize the tool: initialize window IDs, load data, etc...
        // NOTE: Derived classes MUST call base implementation!
        virtual void Initialize( UpdateContext const& context );

        // Shutdown the tool, free allocated memory, etc...
        virtual void Shutdown( UpdateContext const& context );

        // Frame update and draw any tool windows needed for the tool
        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) {}

    protected:

        // Require derived tools to define a ctor
        EditorTool( ToolsContext const* pToolsContext, String const& displayName );

        // Set the tool tab-title (pass by value is intentional)
        virtual void SetDisplayName( String name );

        // Tool Windows
        //-------------------------------------------------------------------------

        ImGuiWindowClass* GetToolWindowClass() { return &m_toolWindowClass; }

        void CreateToolWindow( String const& name, TFunction<void( UpdateContext const&, bool )> const& drawFunction, ImVec2 const& windowPadding = ImVec2( -1, -1 ), bool disableScrolling = false );

        EE_FORCE_INLINE TVector<ToolWindow> const& GetToolWindows() const { return m_toolWindows; }

        EE_FORCE_INLINE InlineString GetToolWindowName( String const& name ) const { return GetToolWindowName( name.c_str(), m_currentDockspaceID ); }

        // Help Menu
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

        // Called before we start a hot-reload operation (before we actually unload anything, allows tools to clean up any dependent data)
        // This is only called if any of the resources that tool has explicitly loaded needs a reload
        virtual void OnHotReloadStarted( TInlineVector<Resource::ResourcePtr*, 10> const& resourcesToBeReloaded ) {}

        // Called after hot-reloading completes (you will always get this call if you get the 'OnHotReloadStarted' call, unless a fatal occurs)
        // This only gets called if we needed to reload anything and the reload was successful
        virtual void OnHotReloadComplete() {}

        // Blocking call to force a recompile of a resource outside of the regular resource server flow, this is needed when you need to ensure a resource has been recompiled in advance of loading
        // Will also flag the resource for hot-reload if there are any current users
        bool RequestImmediateResourceCompilation( ResourceID const& resourceID );

private:

        EditorTool& operator=( EditorTool const& ) = delete;
        EditorTool( EditorTool const& ) = delete;

        // Calculate the dockspace ID for this tool relative to its parent
        inline ImGuiID CalculateDockspaceID()
        {
            int32_t dockspaceID = m_currentLocationID;
            char const* const ptoolTypeName = GetDockingUniqueTypeName();
            dockspaceID = ImHashData( ptoolTypeName, strlen( ptoolTypeName ), dockspaceID );
            return dockspaceID;
        }

        // Draw the workspace windows and help menus
        void DrawSharedMenus();

        // Hot-reload
        //-------------------------------------------------------------------------

        virtual void HotReload_UnloadResources( TVector<Resource::ResourceRequesterID> const& usersToBeReloaded, TVector<ResourceID> const& resourcesToBeReloaded );
        virtual void HotReload_ReloadResources();
        virtual void HotReload_ReloadComplete();

    protected:

        ImGuiID                                     m_ID = 0; // Document identifier (unique)
        ToolsContext const*                         m_pToolsContext = nullptr;
        Resource::ResourceSystem*                   m_pResourceSystem = nullptr;
        String                                      m_windowName;
        DialogManager                               m_dialogManager;

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
        bool                                        m_isHotReloading = false;

        // Hot-reloading
        TVector<Resource::ResourcePtr*>             m_requestedResources;
        TVector<Resource::ResourcePtr*>             m_reloadingResources;
    };
}

//-------------------------------------------------------------------------

#define EE_EDITOR_TOOL( TypeName ) \
constexpr static uint32_t const s_toolTypeID = Hash::FNV1a::GetHash32( #TypeName ); \
constexpr static bool const s_isSingleton = false; \
virtual uint32_t GetToolTypeID() const override final { return TypeName::s_toolTypeID; }

//-------------------------------------------------------------------------

#define EE_SINGLETON_EDITOR_TOOL( TypeName ) \
constexpr static uint32_t const s_toolTypeID = Hash::FNV1a::GetHash32( #TypeName ); \
constexpr static bool const s_isSingleton = true; \
virtual uint32_t GetToolTypeID() const override final { return TypeName::s_toolTypeID; }\
virtual bool IsSingleton() const override final { return true; }