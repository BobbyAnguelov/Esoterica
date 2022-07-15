#pragma once

#include "EditorWorkspace.h"
#include "EngineTools/Core/Helpers/GlobalRegistryBase.h"
#include "EngineTools/Core/PropertyGrid/PropertyGrid.h"
#include "Engine/ToolsUI/Gizmo.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/Resource/ResourceSystem.h"
#include "System/Serialization/JsonSerialization.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace Resource { struct ResourceDescriptor; }
    class ResourceDescriptorUndoableAction;

    //-------------------------------------------------------------------------
    // Generic Resource Workspace
    //-------------------------------------------------------------------------
    // Created for any resources without a custom workspace associated

    class EE_ENGINETOOLS_API GenericResourceWorkspace : public EditorWorkspace
    {
        friend class ScopedDescriptorModification;
        friend ResourceDescriptorUndoableAction;

    public:

        GenericResourceWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID );
        ~GenericResourceWorkspace();

    protected:

        virtual uint32_t GetID() const override { return m_descriptorID.GetPathID(); }
        virtual bool HasViewportWindow() const override { return false; }
        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID ) const override;
        virtual void UpdateWorkspace( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused ) override;
        virtual void DrawWorkspaceToolbarItems( UpdateContext const& context );
        virtual bool IsDirty() const override { return m_isDirty; }
        virtual bool Save() override;
        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) override { m_descriptorPropertyGrid.MarkDirty(); }

        // Undo/Redo
        void PreEdit( PropertyEditInfo const& info );
        void PostEdit( PropertyEditInfo const& info );
        void BeginModification();
        void EndModification();
        void MarkDirty() { m_isDirty = true; }

        // Draws the descriptor property grid editor window - return true if focused
        bool DrawDescriptorEditorWindow( UpdateContext const& context, ImGuiWindowClass* pWindowClass );

        template<typename T>
        T* GetDescriptorAs() { return Cast<T>( m_pDescriptor ); }

        virtual void SerializeCustomDescriptorData( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& descriptorObjectValue ) {}
        virtual void SerializeCustomDescriptorData( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) {}

        virtual void BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToBeReloaded, TVector<ResourceID> const& resourcesToBeReloaded ) override;
        virtual void EndHotReload() override;

    private:

        void LoadDescriptor();


    protected:

        String                                  m_descriptorWindowName;
        ResourceID                              m_descriptorID;
        FileSystem::Path                        m_descriptorPath;
        PropertyGrid                            m_descriptorPropertyGrid;
        Resource::ResourceDescriptor*           m_pDescriptor = nullptr;
        EventBindingID                          m_preEditEventBindingID;
        EventBindingID                          m_postEditEventBindingID;
        ResourceDescriptorUndoableAction*       m_pActiveUndoableAction = nullptr;
        int32_t                                 m_beginModificationCallCount = 0;
        ImGuiX::Gizmo                           m_gizmo;

    private:

        bool                                    m_isDirty = false;
    };

    class [[nodiscard]] ScopedDescriptorModification
    {
    public:

        ScopedDescriptorModification( GenericResourceWorkspace* pWorkspace )
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

        GenericResourceWorkspace*  m_pWorkspace = nullptr;
    };

    //-------------------------------------------------------------------------
    // Resource Editor Workspace
    //-------------------------------------------------------------------------
    // This is a base class to create a sub-editor for a given resource type that runs within the resource editor

    template<typename T>
    class TResourceWorkspace : public GenericResourceWorkspace
    {
        static_assert( std::is_base_of<Resource::IResource, T>::value, "T must derived from IResource" );

    public:

        // Specify whether to initially load the resource, this is not necessary for all editors
        TResourceWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID, bool shouldLoadResource = true )
            : GenericResourceWorkspace( pToolsContext, pWorld, resourceID )
            , m_pResource( resourceID )
        {
            EE_ASSERT( resourceID.IsValid() );

            if ( shouldLoadResource )
            {
                LoadResource( &m_pResource );
            }
        }

        ~TResourceWorkspace()
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

        String                              m_title;
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

        static bool HasCustomWorkspace( ResourceTypeID const& resourceTypeID );
        static EditorWorkspace* TryCreateWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID );

    protected:

        // Get the resource type this factory supports
        virtual ResourceTypeID GetSupportedResourceTypeID() const = 0;

        // Virtual method that will create a workspace if the resource ID matches the appropriate types
        virtual EditorWorkspace* CreateWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID ) const = 0;
    };
}

//-------------------------------------------------------------------------
//  Macro to create a resource workspace factory
//-------------------------------------------------------------------------
// Use in a CPP to define a factory e.g., EE_RESOURCE_WORKSPACE_FACTORY( SkeletonWorkspaceFactory, Skeleton, SkeletonResourceEditor );
    
#define EE_RESOURCE_WORKSPACE_FACTORY( factoryName, resourceClass, workspaceClass )\
class factoryName final : public ResourceWorkspaceFactory\
{\
    virtual ResourceTypeID GetSupportedResourceTypeID() const override { return resourceClass::GetStaticResourceTypeID(); }\
    virtual EditorWorkspace* CreateWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID ) const override\
    {\
        EE_ASSERT( resourceID.GetResourceTypeID() == resourceClass::GetStaticResourceTypeID() );\
        return EE::New<workspaceClass>( pToolsContext, pWorld, resourceID );\
    }\
};\
static factoryName g_##factoryName;