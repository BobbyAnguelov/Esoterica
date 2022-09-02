#include "Workspace_EntityCollectionEditor.h"
#include "EngineTools/Entity/EntitySerializationTools.h"
#include "Engine/UpdateContext.h"
#include "Engine/Entity/EntitySerialization.h"
#include "System/Threading/TaskSystem.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    EE_RESOURCE_WORKSPACE_FACTORY( EntityCollectionEditorFactory, SerializedEntityCollection, EntityCollectionEditor );

    //-------------------------------------------------------------------------

    EntityCollectionEditor::EntityCollectionEditor( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& collectionResourceID )
        : EntityWorldEditorWorkspace( pToolsContext, pWorld, collectionResourceID )
        , m_collection( collectionResourceID )
    {
        SetDisplayName( collectionResourceID.GetFileNameWithoutExtension() );
    }

    //-------------------------------------------------------------------------

    void EntityCollectionEditor::Initialize( UpdateContext const& context )
    {
        EntityWorldEditorWorkspace::Initialize( context );
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

        EntityWorldEditorWorkspace::Shutdown( context );
    }

    //-------------------------------------------------------------------------

    bool EntityCollectionEditor::Save()
    {
        auto pEditedMap = m_context.GetMap();
        if ( pEditedMap == nullptr || !( pEditedMap->IsLoaded() || pEditedMap->IsActivated() ) )
        {
            return false;
        }

        SerializedEntityMap sem;
        if ( !Serializer::SerializeEntityMap( m_context.GetTypeRegistry(), pEditedMap, sem ) )
        {
            return false;
        }

        FileSystem::Path const filePath = GetFileSystemPath( m_collection.GetResourcePath() );
        return WriteSerializedEntityCollectionToFile( m_context.GetTypeRegistry(), sem, filePath );
    }

    //-------------------------------------------------------------------------

    void EntityCollectionEditor::Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused )
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

        EntityWorldEditorWorkspace::Update( context, pWindowClass, isFocused );
    }
}