#pragma once
#include "Base/TypeSystem/TypeID.h"
#include "Engine/Entity/EntityIDs.h"
#include "Engine/Entity/Entity.h"
#include "Base/Encoding/Hash.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EntityEditorItem
    {
    public:

        static inline uint64_t CalculateUniqueID( Entity* pEntity ) { return pEntity->GetID().m_value; }
        static inline uint64_t CalculateUniqueID( Entity* pEntity, EntityComponent* pEntityComponent ) { return Hash::CombineHashes64( pEntity->GetID().m_value, pEntityComponent->GetNameID().ToUint() ); }
        static inline uint64_t CalculateUniqueID( Entity* pEntity, EntitySystem* pEntitySystem ) { return Hash::CombineHashes64( pEntity->GetID().m_value, pEntitySystem->GetTypeInfo()->m_ID.ToUint() ); }

    public:

        EntityEditorItem() = default;

        EntityEditorItem( Entity* pEntity );
        EntityEditorItem( Entity* pEntity, EntityComponent* pComponent );
        EntityEditorItem( Entity* pEntity, EntitySystem* pEntitySystem );

        bool IsValid() const;
        inline void Reset() { *this = EntityEditorItem(); }

        inline uint64_t GetUniqueID() const { return m_uniqueID; }
        char const* GetTypeFriendlyName() const;

        inline bool IsEntity() const { return m_pComponent == nullptr && m_pSystem == nullptr; }
        inline bool IsSpatialEntity() const { return IsEntity() && m_pEntity->IsSpatialEntity(); }
        inline bool IsComponent() const { return m_pComponent != nullptr; }
        inline bool IsSpatialComponent() const { return IsComponent() && m_isSpatialComponent; }
        inline bool IsSystem() const { return m_pSystem != nullptr; }

        SpatialEntityComponent* GetSpatialComponent() { EE_ASSERT( IsSpatialComponent() ); return Cast<SpatialEntityComponent>( m_pComponent ); }
        SpatialEntityComponent const* GetSpatialComponent() const { EE_ASSERT( IsSpatialComponent() ); return Cast<SpatialEntityComponent>( m_pComponent ); }

        inline bool operator==( EntityEditorItem const& rhs ) const { return m_uniqueID == rhs.m_uniqueID; }
        inline bool operator!=( EntityEditorItem const& rhs ) const { return m_uniqueID != rhs.m_uniqueID; }

        void UpdateEntityPtrs( EntityWorld* pWorld );

    public:

        Entity*                         m_pEntity = nullptr;
        EntityID                        m_entityID; // Cached since we want to access this on undo/redo/etc without touching the entity memory which might have been invalidated
        EntityComponent*                m_pComponent = nullptr;
        ComponentID                     m_componentID; // Cached since we want to access this on undo/redo/etc without touching the entity memory which might have been invalidated
        EntitySystem*                   m_pSystem = nullptr;
        TypeSystem::TypeID              m_systemTypeID; // Cached since we want to access this on undo/redo/etc without touching the entity memory which might have been invalidated
        bool                            m_isSpatialComponent = false;

    private:

        uint64_t                        m_uniqueID = 0;
    };

    //-------------------------------------------------------------------------

    struct CategorizedEntityItemTypeInfo
    {
        StringID                                m_name;
        TVector<TypeSystem::TypeInfo const*>    m_typeInfos;
    };

    //-------------------------------------------------------------------------

    struct EntityEditorItemGroup
    {
        static bool IsParentOf( StringID parentID, StringID childID );
        static bool IsDirectParentOf( StringID parentID, StringID childID );

    public:

        EntityEditorItemGroup() = default;
        EntityEditorItemGroup( StringID ID ) : m_ID( ID ) { EE_ASSERT( m_ID.IsValid() ); }

        inline bool IsValid() const { return m_ID.IsValid(); }
        void UpdateEntityPtrs( EntityWorld* pWorld );

        bool IsParentOf( StringID childID ) const { return IsParentOf( m_ID, childID ); }
        bool IsDirectParentOf( StringID childID ) const { return IsDirectParentOf( m_ID, childID ); }
        bool IsChildOf( StringID parentID ) const { return IsParentOf( parentID, m_ID ); }
        bool IsDirectChildOf( StringID parentID ) const { return IsDirectParentOf( parentID, m_ID ); }

        void ReplaceParent( StringID newParentID );

    public:

        StringID                        m_ID;
        TVector<EntityEditorItem>       m_items;
    };
}