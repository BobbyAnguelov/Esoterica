#pragma once

#include "Engine/Entity/EntityWorldSystem.h"
#include "Engine/UpdateContext.h"
#include "Engine/Entity/EntityWorldSystemSignal.h"
#include "Base/Threading/Threading.h"
#include "Base/Systems.h"
#include "Base/Types/IDVector.h"
#include "Base/Types/Event.h"

//-------------------------------------------------------------------------

namespace EE
{
    class ActorComponent;
}

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class PhysicsShapeComponent;
    class PhysicsTestComponent;
    class PhysicsWorld;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API PhysicsWorldSystem final : public EntityWorldSystem
    {
        friend class PhysicsDebugView;

        struct EntityPhysicsRecord
        {
            inline bool IsEmpty() const { return m_components.empty(); }

            TVector<PhysicsShapeComponent*>                     m_components;
        };

    public:

        EE_ENTITY_WORLD_SYSTEM( PhysicsWorldSystem, RequiresUpdate( UpdateStage::Physics ), RequiresUpdate( UpdateStage::PostPhysics ) );

    public:

        PhysicsWorldSystem() = default;

        PhysicsWorld const* GetPhysicsWorld() const { return m_pPhysicsWorld; }
        PhysicsWorld* GetPhysicsWorld() { return m_pPhysicsWorld; }

    private:

        virtual void InitializeSystem( SystemRegistry const& systemRegistry ) override;
        virtual void ShutdownSystem() override final;
        virtual void RegisterComponent( Entity* pEntity, EntityComponent* pComponent ) override final;
        virtual void UnregisterComponent( Entity* pEntity, EntityComponent* pComponent ) override final;
        virtual void UpdateSystem( EntityWorldUpdateContext const& ctx ) override final;

        void RegisterShapeComponent( PhysicsShapeComponent* pComponent );
        void UnregisterShapeComponent( PhysicsShapeComponent* pComponent );

        void ProcessBodyRebuildRequests( EntityWorldUpdateContext const& ctx );

        void PhysicsUpdate( EntityWorldUpdateContext const& ctx );
        void PostPhysicsUpdate( EntityWorldUpdateContext const& ctx );

        #if EE_DEVELOPMENT_TOOLS
        bool IsRecording() const;
        void StartRecording();
        void StopRecording();
        virtual void DebugDraw( EntityWorldUpdateContext const& ctx ) override;
        #endif

    private:

        PhysicsWorld*                                           m_pPhysicsWorld = nullptr;

        TIDVector<ComponentID, PhysicsShapeComponent*>          m_physicsShapeComponents;
        TIDVector<ComponentID, PhysicsShapeComponent*>          m_dynamicShapeComponents;
        TIDVector<ComponentID, PhysicsShapeComponent*>          m_kinematicShapeComponents;

        TEntityMessageQueue<PhysicsShapeComponent>              m_bodyRebuildRequests;

        TVector<PhysicsTestComponent*>                          m_testComponents;
    };
}