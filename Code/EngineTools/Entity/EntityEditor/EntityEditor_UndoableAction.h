#pragma once
#include "EngineTools/Core/UndoStack.h"
#include "Engine/Entity/EntityIDs.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "EntityEditor_EntityItem.h"
#include "Base/Math/Transform.h"
#include "EngineTools/Entity/EntityToolTypes.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EditorContext;

    //-------------------------------------------------------------------------

    class EntityUndoableAction final : public IUndoableAction
    {
        EE_REFLECT_TYPE( EntityUndoableAction );

        struct SerializedEntity
        {
            EntityMapID                         m_mapID;
            EntityDescriptor                    m_desc;
        };

        struct SerializedComponentTransform
        {
            EntityID                            m_entityID;
            ComponentID                         m_componentID;
            Transform                           m_transform;
            Float3                              m_nonUniformScale;
        };

    public:

        enum Type
        {
            Invalid = 0,
            CreateEntities,
            DeleteEntities,
            ModifyEntities,
            TransformUpdate,
        };

    public:

        EntityUndoableAction() = default;
        EntityUndoableAction( EditorContext* pContext );

        Type GetActionType() const { return m_actionType; }

        //-------------------------------------------------------------------------

        void RecordCreate( Entity* pEntity ) { RecordCreate( &pEntity, 1 ); }

        void RecordCreate( TVector<Entity*>& createdEntities ) { RecordCreate( createdEntities.data(), createdEntities.size() ); }

        template<size_t S>
        void RecordCreate( TInlineVector<Entity*, S>& createdEntities ) { RecordCreate( createdEntities.data(), createdEntities.size() ); }

        //-------------------------------------------------------------------------

        void RecordDelete( Entity* pEntity ) { RecordDelete( &pEntity, 1 ); }

        void RecordDelete( TVector<Entity*>& entitiesToDelete ) { RecordDelete( entitiesToDelete.data(), entitiesToDelete.size() ); }

        template<size_t S>
        void RecordDelete( TInlineVector<Entity*, S>& entitiesToDelete ) { RecordDelete( entitiesToDelete.data(), entitiesToDelete.size() ); }

        //-------------------------------------------------------------------------

        void RecordBeginEdit( EntityEditorItem& entityItem ) { RecordBeginEdit( &entityItem, 1 ); }

        void RecordBeginEdit( EntityEditorItem&& entityItem ) { RecordBeginEdit( &entityItem, 1 ); }

        void RecordBeginEdit( Entity* pEntity ) { EntityEditorItem ei( pEntity ); RecordBeginEdit( ei ); }

        void RecordBeginEdit( TVector<EntityEditorItem>& itemsToBeModified ) { RecordBeginEdit( itemsToBeModified.data(), itemsToBeModified.size() ); }

        template<size_t S>
        void RecordBeginEdit( TInlineVector<EntityEditorItem, S>& itemsToBeModified ) { RecordBeginEdit( itemsToBeModified.data(), itemsToBeModified.size() ); }

        void RecordEndEdit();

        //-------------------------------------------------------------------------

        void RecordBeginTransformEdit( EntityEditorItem& entityItem ) { RecordBeginTransformEdit( &entityItem, 1 ); }

        void RecordBeginTransformEdit( EntityEditorItem&& entityItem ) { RecordBeginTransformEdit( &entityItem, 1 ); }

        void RecordBeginTransformEdit( Entity* pEntity ) { EntityEditorItem ei( pEntity ); RecordBeginTransformEdit( ei ); }

        void RecordBeginTransformEdit( TVector<EntityEditorItem>& itemsToBeModified ) { RecordBeginTransformEdit( itemsToBeModified.data(), itemsToBeModified.size() ); }

        template<size_t S>
        void RecordBeginTransformEdit( TInlineVector<EntityEditorItem, S>& itemsToBeModified ) { RecordBeginTransformEdit( itemsToBeModified.data(), itemsToBeModified.size() ); }

        void RecordEndTransformEdit();

        //-------------------------------------------------------------------------

        TVector<EntityEditorItem> const& GetRecordedSelection( UndoStack::Operation operation ) const;

    private:

        virtual void Undo() override;
        virtual void Redo() override;

        void UpdateSpatialParent( EntityMapID mapID, EntityDescriptor const& desc ) const;

        void RecordCreate( Entity** pEntities, size_t numEntities );
        void RecordDelete( Entity** pEntities, size_t numEntities );
        void RecordBeginEdit( EntityEditorItem* pItemsToBeModified, size_t numItems );
        void RecordBeginTransformEdit( EntityEditorItem* pItemsToBeModified, size_t numItems );

    private:

        EditorContext*                                      m_pContext = nullptr;
        TypeSystem::TypeRegistry const*                     m_pTypeRegistry = nullptr;
        EntityWorld*                                        m_pWorld = nullptr;
        Type                                                m_actionType = Invalid;
        TVector<EntityEditorItem>                           m_preModificationSelection;
        TVector<EntityEditorItem>                           m_postModificationSelection;

        // Data: Create
        TVector<SerializedEntity>                           m_createdEntities;

        // Data: Delete
        TVector<SerializedEntity>                           m_deletedEntities;

        // Data: Modify
        TVector<Entity*>                                    m_editedEntities; // Temporary storage that is only valid between a Begin and End call
        TVector<SerializedEntity>                           m_entityDescPreModification;
        TVector<SerializedEntity>                           m_entityDescPostModification;
        TVector<SerializedComponentTransform>               m_transformsPreModification;
        TVector<SerializedComponentTransform>               m_transformsPostModification;

        Log                                                 m_log;
    };

    //-------------------------------------------------------------------------

    class EntityGroupsUndoableAction final : public IUndoableAction
    {
        EE_REFLECT_TYPE( EntityGroupsUndoableAction );

    public:

        EntityGroupsUndoableAction() = default;
        EntityGroupsUndoableAction( EditorContext* pContext ) : m_pContext( pContext ) { EE_ASSERT( m_pContext != nullptr ); }

        void Begin();
        void End();

    private:

        virtual void Undo() override;
        virtual void Redo() override;

    private:

        EditorContext*                                             m_pContext = nullptr;
        THashMap<EntityMapID, TVector<EntityEditorItemGroup>>      m_preModificationGroups;
        THashMap<EntityMapID, TVector<EntityEditorItemGroup>>      m_postModificationGroups;
    };
}