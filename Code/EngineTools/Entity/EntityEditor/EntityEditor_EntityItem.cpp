#include "EntityEditor_EntityItem.h"
#include "Engine/Entity/EntityWorld.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    EntityEditorItem::EntityEditorItem( Entity* pEntity )
        : m_pEntity( pEntity )
        , m_entityID( pEntity->GetID() )
        , m_uniqueID( CalculateUniqueID( m_pEntity ) )
    {
        EE_ASSERT( m_pEntity != nullptr );
    }

    EntityEditorItem::EntityEditorItem( Entity* pEntity, EntityComponent* pComponent )
        : m_pEntity( pEntity )
        , m_entityID( pEntity->GetID() )
        , m_pComponent( pComponent )
        , m_componentID( pComponent->GetID() )
        , m_isSpatialComponent( TryCast<SpatialEntityComponent>( pComponent ) != nullptr )
        , m_uniqueID( CalculateUniqueID( m_pEntity, m_pComponent ) )
    {
        EE_ASSERT( m_pEntity != nullptr && m_pComponent != nullptr );
    }

    EntityEditorItem::EntityEditorItem( Entity* pEntity, EntitySystem* pEntitySystem )
        : m_pEntity( pEntity )
        , m_entityID( pEntity->GetID() )
        , m_pSystem( pEntitySystem )
        , m_systemTypeID( pEntitySystem->GetTypeID() )
        , m_uniqueID( CalculateUniqueID( m_pEntity, m_pSystem ) )
    {
        EE_ASSERT( m_pEntity != nullptr && m_pSystem != nullptr );
    }

    bool EntityEditorItem::IsValid() const
    {
        if ( m_pEntity == nullptr )
        {
            return false;
        }

        if ( m_componentID.IsValid() && m_pComponent == nullptr )
        {
            return false;
        }

        if ( m_systemTypeID.IsValid() && m_pSystem == nullptr )
        {
            return false;
        }

        return true;
    }

    void EntityEditorItem::UpdateEntityPtrs( EntityWorld* pWorld )
    {
        m_pEntity = pWorld->FindEntity( m_entityID );
        if ( m_pEntity == nullptr )
        {
            m_pComponent = nullptr;
            m_pSystem = nullptr;
            return;
        }

        //-------------------------------------------------------------------------

        if ( m_componentID.IsValid() )
        {
            m_pComponent = m_pEntity->FindComponent( m_componentID );
        }

        if ( m_systemTypeID.IsValid() )
        {
            m_pSystem = m_pEntity->GetSystem( m_systemTypeID );
        }
    }

    char const* EntityEditorItem::GetTypeFriendlyName() const
    {
        if ( IsEntity() )
        {
            m_pEntity->GetTypeInfo()->GetFriendlyTypeName();
        }
        else if ( IsComponent() )
        {
            m_pComponent->GetTypeInfo()->GetFriendlyTypeName();
        }
        else if ( IsSystem() )
        {
            m_pSystem->GetTypeInfo()->GetFriendlyTypeName();
        }

        return "";
    }

    //-------------------------------------------------------------------------

    void EntityEditorItemGroup::UpdateEntityPtrs( EntityWorld* pWorld )
    {
        for ( int32_t i = (int32_t) m_items.size() - 1; i >= 0; i-- )
        {
            m_items[i].UpdateEntityPtrs( pWorld );
            if ( !m_items[i].IsValid() )
            {
                m_items.erase( m_items.begin() + i );
            }
        }
    }

    bool EntityEditorItemGroup::IsParentOf( StringID parentID, StringID childID )
    {
        EE_ASSERT( parentID.IsValid() && childID.IsValid() );

        if ( parentID == childID )
        {
            return false;
        }

        InlineString const childStr( childID.c_str() );
        return childStr.find( parentID.c_str() ) == 0;
    }

    bool EntityEditorItemGroup::IsDirectParentOf( StringID parentID, StringID childID )
    {
        EE_ASSERT( parentID.IsValid() && childID.IsValid() );
        
        if ( parentID == childID )
        {
            return false;
        }

        InlineString const childStr( childID.c_str() );
        if ( childStr.find( parentID.c_str() ) != 0 )
        {
            return false;
        }

        size_t const foundDelimiterIdx = childStr.find_first_of( '/', strlen( parentID.c_str() ) + 1 );
        return foundDelimiterIdx == String::npos;
    } 

    void EntityEditorItemGroup::ReplaceParent( StringID newParentID )
    {
        EE_ASSERT( IsValid() );

        InlineString parentStr;
        if( newParentID.IsValid() )
        {
            parentStr.append( newParentID.c_str() );
            if ( parentStr.back() != '/' )
            {
                parentStr.append( "/" );
            }
        }

        //-------------------------------------------------------------------------

        InlineString childStr( m_ID.c_str() );
        size_t const delimiterIdx = childStr.find_last_of( '/' );
        if ( delimiterIdx == InlineString::npos )
        {
            parentStr.append( childStr );
        }
        else
        {
            parentStr.append( childStr.substr( delimiterIdx + 1, childStr.length() - delimiterIdx ) );
        }

        m_ID = StringID( parentStr.c_str() );
    }
}