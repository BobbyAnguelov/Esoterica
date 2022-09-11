#pragma once
#include "EngineTools/Core/UndoStack.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Engine/Entity/EntityIDs.h"

//-------------------------------------------------------------------------

namespace EE
{
    class Entity;
    class EntityWorld;
    namespace TypeSystem { class TypeRegistry; }
}

namespace EE::EntityModel
{
    class EntityUndoableAction final : public IUndoableAction
    {
        struct SerializedEntity
        {
            EntityMapID                         m_mapID;
            SerializedEntityDescriptor          m_desc;
        };

    public:

        enum Type
        {
            Invalid = 0,
            CreateEntities,
            DeleteEntities,
            ModifyEntities,
        };

    public:

        EntityUndoableAction( TypeSystem::TypeRegistry const& typeRegistry, EntityWorld* pWorld );

        void RecordCreate( TVector<Entity*> createdEntities );
        void RecordDelete( TVector<Entity*> entitiesToDelete );
        void RecordBeginEdit( TVector<Entity*> entitiesToBeModified, bool wereEntitiesDuplicated = false );
        void RecordEndEdit();

    private:

        void GenerateFullModifiedEntityList();

        virtual void Undo() override;
        virtual void Redo() override;

    private:

        TypeSystem::TypeRegistry const&         m_typeRegistry;
        EntityWorld*                            m_pWorld = nullptr;
        Type                                    m_actionType = Invalid;

        // Data: Create
        TVector<SerializedEntity>               m_createdEntities;

        // Data: Delete
        TVector<SerializedEntity>               m_deletedEntities;

        // Data: Modify
        TVector<Entity*>                        m_editedEntities; // Temporary storage that is only valid between a Begin and End call
        TVector<SerializedEntity>               m_entityDescPreModification;
        TVector<SerializedEntity>               m_entityDescPostModification;
        bool                                    m_entitiesWereDuplicated = false;
    };
}