#pragma once

#include "Engine/Entity/EntityWorldSystem.h"
#include "Engine/UpdateContext.h"
#include "System/Threading/Threading.h"
#include "System/Systems.h"
#include "System/Math/Transform.h"
#include "System/Types/IDVector.h"
#include "System/Types/ScopedValue.h"
#include "System/Types/Event.h"

//-------------------------------------------------------------------------

namespace EE
{
    struct AABB;
}

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class PhysicsShapeComponent;
    class CharacterComponent;
    class PhysicsTestComponent;
    class PhysicsWorld;
    enum class DynamicMotionType;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API PhysicsWorldSystem final : public IEntityWorldSystem
    {
        friend class PhysicsDebugView;

        struct EntityPhysicsRecord
        {
            inline bool IsEmpty() const { return m_components.empty(); }

            TVector<PhysicsShapeComponent*>                     m_components;
        };

    public:

        EE_ENTITY_WORLD_SYSTEM( PhysicsWorldSystem, RequiresUpdate( UpdateStage::Physics ), RequiresUpdate( UpdateStage::PostPhysics ), RequiresUpdate( UpdateStage::Paused ) );

    public:

        PhysicsWorldSystem() = default;

        PhysicsWorld const* GetWorld() const { return m_pWorld; }
        PhysicsWorld* GetWorld() { return m_pWorld; }

    private:

        virtual void InitializeSystem( SystemRegistry const& systemRegistry ) override;
        virtual void ShutdownSystem() override final;
        virtual void RegisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;
        virtual void UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;
        virtual void UpdateSystem( EntityWorldUpdateContext const& ctx ) override final;

        void RegisterDynamicComponent( PhysicsShapeComponent* pComponent );
        void UnregisterDynamicComponent( PhysicsShapeComponent* pComponent );

        void ProcessActorRebuildRequests( EntityWorldUpdateContext const& ctx );

        void PhysicsUpdate( EntityWorldUpdateContext const& ctx );
        void PostPhysicsUpdate( EntityWorldUpdateContext const& ctx );

    private:

        PhysicsWorld*                                           m_pWorld = nullptr;

        TIDVector<ComponentID, CharacterComponent*>             m_characterComponents;
        TIDVector<ComponentID, PhysicsShapeComponent*>          m_physicsShapeComponents;
        TIDVector<ComponentID, PhysicsShapeComponent*>          m_dynamicShapeComponents; // TODO: profile and see if we need to use a dynamic pool

        EventBindingID                                          m_actorRebuildBindingID;
        Threading::Mutex                                        m_mutex;
        TVector<PhysicsShapeComponent*>                         m_actorRebuildRequests;

        TVector<PhysicsTestComponent*>                          m_testComponents;
    };
}