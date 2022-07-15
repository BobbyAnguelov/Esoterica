#include "Workspace_EntityCollectionEditor.h"
#include "Engine/Entity/EntitySerialization.h"
#include "System/Threading/TaskSystem.h"
#include "Engine/UpdateContext.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    EntityCollectionEditor::EntityCollectionEditor( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& collectionResourceID )
        : EntityEditorBaseWorkspace( pToolsContext, pWorld )
        , m_collection( collectionResourceID )
    {
        SetDisplayName( collectionResourceID.GetFileNameWithoutExtension() );
    }

    //-------------------------------------------------------------------------

    void EntityCollectionEditor::Initialize( UpdateContext const& context )
    {
        EntityEditorBaseWorkspace::Initialize( context );
        LoadResource( &m_collection );
        m_collectionInstantiated = false;
    }

    void EntityCollectionEditor::Shutdown( UpdateContext const& context )
    {
        if ( m_collection.IsLoaded() )
        {
            UnloadResource( &m_collection );
        }

        m_collectionInstantiated = false;

        EntityEditorBaseWorkspace::Shutdown( context );
    }

    //-------------------------------------------------------------------------

    bool EntityCollectionEditor::Save()
    {
        auto pEditedMap = m_context.GetMap();
        if ( pEditedMap == nullptr || !( pEditedMap->IsLoaded() || pEditedMap->IsActivated() ) )
        {
            return false;
        }

        EntityCollectionDescriptor ecd;
        if ( !pEditedMap->CreateDescriptor( m_context.GetTypeRegistry(), ecd) )
        {
            return false;
        }

        FileSystem::Path const filePath = GetFileSystemPath( m_collection.GetResourcePath() );
        return Serializer::WriteEntityCollectionToFile( m_context.GetTypeRegistry(), ecd, filePath);
    }

    //-------------------------------------------------------------------------

    void EntityCollectionEditor::UpdateWorkspace( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused )
    {
        if ( !m_collectionInstantiated )
        {
            if ( m_collection.IsLoaded() )
            {
                // Create transient map for the collection editing
                auto pMap = m_pWorld->CreateTransientMap();
                pMap->AddEntityCollection( context.GetSystem<TaskSystem>(), m_context.GetTypeRegistry(), *m_collection.GetPtr() );
                
                // Unload the collection resource
                m_collectionInstantiated = true;
                UnloadResource( &m_collection );

                // Set the context to operate on the new map
                m_context.SetMapToUse( pMap->GetID() );
            }
        }

        EntityEditorBaseWorkspace::UpdateWorkspace( context, pWindowClass, isFocused );
    }
}