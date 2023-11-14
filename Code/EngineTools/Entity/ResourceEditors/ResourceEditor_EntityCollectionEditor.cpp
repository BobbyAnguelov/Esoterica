#include "ResourceEditor_EntityCollectionEditor.h"
#include "EngineTools/Entity/EntitySerializationTools.h"
#include "Engine/UpdateContext.h"
#include "Engine/Entity/EntitySerialization.h"
#include "Engine/Entity/EntityWorld.h"
#include "Base/Threading/TaskSystem.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    EE_RESOURCE_EDITOR_FACTORY( EntityCollectionEditorFactory, SerializedEntityCollection, EntityCollectionEditor );

    //-------------------------------------------------------------------------

    EntityCollectionEditor::EntityCollectionEditor( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& collectionResourceID )
        : EntityEditor( pToolsContext, pWorld, collectionResourceID )
        , m_collection( collectionResourceID )
    {
        SetDisplayName( collectionResourceID.GetFileNameWithoutExtension() );
    }

    //-------------------------------------------------------------------------

    void EntityCollectionEditor::Initialize( UpdateContext const& context )
    {
        EntityEditor::Initialize( context );
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

        EntityEditor::Shutdown( context );
    }

    //-------------------------------------------------------------------------

    bool EntityCollectionEditor::SaveData()
    {
        auto pEditedMap = m_pWorld->GetFirstNonPersistentMap();
        if ( pEditedMap == nullptr || !pEditedMap->IsLoaded() )
        {
            return false;
        }

        SerializedEntityMap sem;
        if ( !Serializer::SerializeEntityMap( *m_pToolsContext->m_pTypeRegistry, pEditedMap, sem ) )
        {
            return false;
        }

        FileSystem::Path const filePath = GetFileSystemPath( m_collection.GetResourcePath() );
        if ( !WriteSerializedEntityCollectionToFile( *m_pToolsContext->m_pTypeRegistry, sem, filePath ) )
        {
            return false;
        }

        ClearDirty();
        return true;
    }

    //-------------------------------------------------------------------------

    void EntityCollectionEditor::DrawMenu( UpdateContext const& context )
    {
        EntityEditor::DrawMenu( context );

        if ( ImGui::BeginMenu( EE_ICON_TUNE" Options" ) )
        {
            ImGuiX::Checkbox( "Draw Grid", &m_drawGrid );
            ImGui::EndMenu();
        }
    }

    //-------------------------------------------------------------------------

    void EntityCollectionEditor::Update( UpdateContext const& context, bool isVisible, bool isFocused )
    {
        if ( !m_collectionInstantiated )
        {
            if ( m_collection.IsLoaded() )
            {
                // Create transient map for the collection editing
                auto pMap = m_pWorld->CreateTransientMap();
                pMap->AddEntityCollection( context.GetSystem<TaskSystem>(), *m_pToolsContext->m_pTypeRegistry, *m_collection.GetPtr() );
                
                // Unload the collection resource
                m_collectionInstantiated = true;
                UnloadResource( &m_collection );
            }
        }

        EntityEditor::Update( context, isVisible, isFocused );

        // Draw Grid
        //-------------------------------------------------------------------------

        if ( m_drawGrid )
        {
            constexpr float const lineLength = 20;
            constexpr float const halfLineLength = lineLength / 2.0f;

            auto drawingCtx = GetDrawingContext();
            for ( float i = -halfLineLength; i <= halfLineLength; i++ )
            {
                drawingCtx.DrawLine( Vector( -halfLineLength, i, 0.0f ), Vector( halfLineLength, i, 0.0f ), Colors::LightGray, 1.0f, Drawing::DepthTest::Enable );
                drawingCtx.DrawLine( Vector( i, -halfLineLength, 0.0f ), Vector( i, halfLineLength, 0.0f ), Colors::LightGray, 1.0f, Drawing::DepthTest::Enable );
            }
        }
    }
}