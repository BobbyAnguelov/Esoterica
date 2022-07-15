#include "EntityEditor_PropertyGrid.h"
#include "EntityEditor_Context.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    EntityEditorPropertyGrid::EntityEditorPropertyGrid( EntityEditorContext& ctx )
        : m_context( ctx )
        , m_propertyGrid( ctx.m_pToolsContext )
    {
        m_preEditBindingID = m_propertyGrid.OnPreEdit().Bind( [this] ( PropertyEditInfo const& eventInfo ) { PreEdit( eventInfo ); } );
        m_postEditBindingID = m_propertyGrid.OnPostEdit().Bind( [this] ( PropertyEditInfo const& eventInfo ) { PostEdit( eventInfo ); } );
    }

    EntityEditorPropertyGrid::~EntityEditorPropertyGrid()
    {
        m_propertyGrid.OnPreEdit().Unbind( m_preEditBindingID );
        m_propertyGrid.OnPostEdit().Unbind( m_postEditBindingID );
    }

    void EntityEditorPropertyGrid::PreEdit( PropertyEditInfo const& eventInfo )
    {
        if ( auto pComponent = TryCast<EntityComponent>( eventInfo.m_pEditedTypeInstance ) )
        {
            m_context.BeginEditComponent( pComponent );
        }
        else if ( auto pEntity = TryCast<Entity>( eventInfo.m_pEditedTypeInstance ) )
        {
            m_context.BeginEditEntity( pEntity );
        }
    }

    void EntityEditorPropertyGrid::PostEdit( PropertyEditInfo const& eventInfo )
    {
        if ( auto pComponent = TryCast<EntityComponent>( eventInfo.m_pEditedTypeInstance ) )
        {
            // Reset the local transform to ensure that the world transform is recalculated
            if ( eventInfo.m_pPropertyInfo->m_ID == StringID( "m_transform" ) )
            {
                if ( auto pSpatialComponent = TryCast<SpatialEntityComponent>( pComponent ) )
                {
                    pSpatialComponent->SetLocalTransform( pSpatialComponent->GetLocalTransform() );
                }
            }

            m_context.EndEditComponent();
        }
        else if ( auto pEntity = TryCast<Entity>( eventInfo.m_pEditedTypeInstance ) )
        {
            m_context.EndEditEntity();
        }
    }

    void EntityEditorPropertyGrid::Draw( UpdateContext const& context )
    {
        IRegisteredType* pSelectedType = nullptr;

        if ( m_context.HasSingleSelectedComponent() )
        {
            pSelectedType = m_context.GetSelectedComponent();
        }
        else if ( m_context.HasSingleSelectedEntity() )
        {
            pSelectedType = m_context.GetSelectedEntity();
        }

        //-------------------------------------------------------------------------

        if ( m_propertyGrid.GetEditedType() != pSelectedType )
        {
            m_propertyGrid.SetTypeToEdit( pSelectedType );
        }

        //-------------------------------------------------------------------------

        m_propertyGrid.DrawGrid();
    }
}