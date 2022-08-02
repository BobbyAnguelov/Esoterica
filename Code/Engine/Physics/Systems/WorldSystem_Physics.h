#pragma once

#include "Engine/_Module/API.h"

#include "Engine/Entity/EntityWorldSystem.h"
#include "Engine/UpdateContext.h"
#include "System/Systems.h"
#include "System/Types/IDVector.h"
#include "System/Types/ScopedValue.h"
#include "System/Types/Event.h"

//-------------------------------------------------------------------------

namespace physx
{
    class PxPhysics;
    class PxScene;
    class PxRigidActor;
    class PxShape;
}

//-------------------------------------------------------------------------

namespace EE
{
    struct AABB;
}

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class PhysicsSystem;
    class PhysicsShapeComponent;
    class CharacterComponent;
    class Scene;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API PhysicsWorldSystem final : public IEntityWorldSystem
    {
        friend class PhysicsDebugView;
        friend class PhysicsRenderer;

        struct EntityPhysicsRecord
        {
            inline bool IsEmpty() const { return m_components.empty(); }

            TVector<PhysicsShapeComponent*>                  m_components;
        };

    public:

        EE_REGISTER_ENTITY_WORLD_SYSTEM( PhysicsWorldSystem, RequiresUpdate( UpdateStage::Physics ), RequiresUpdate( UpdateStage::PostPhysics ) );

    public:

        PhysicsWorldSystem() = default;
        PhysicsWorldSystem( PhysicsSystem& physicsSystem );

        // Get the scene
        Scene const* GetScene() const { return m_pScene; }

        // Get the scene
        Scene* GetScene() { return m_pScene; }

        // Debug
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        inline uint32_t GetDebugFlags() const { return m_sceneDebugFlags; }
        void SetDebugFlags( uint32_t debugFlags );

        inline bool IsDebugDrawingEnabled() const;
        void SetDebugDrawingEnabled( bool enableDrawing );
        inline float GetDebugDrawDistance() const { return m_debugDrawDistance; }
        inline void SetDebugDrawDistance( float drawDistance ) { m_debugDrawDistance = Math::Max( drawDistance, 0.0f ); }

        void SetDebugCullingBox( AABB const& cullingBox );
        #endif

    private:

        // Get the physx scene
        physx::PxScene* GetPxScene();

        virtual void InitializeSystem( SystemRegistry const& systemRegistry ) override;
        virtual void ShutdownSystem() override final;
        virtual void RegisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;
        virtual void UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;
        virtual void UpdateSystem( EntityWorldUpdateContext const& ctx ) override final;

        bool CreateActorAndShape( PhysicsShapeComponent* pComponent ) const;
        physx::PxRigidActor* CreateActor( PhysicsShapeComponent* pComponent ) const;
        physx::PxShape* CreateShape( PhysicsShapeComponent* pComponent, physx::PxRigidActor* pActor ) const;
        void DestroyActor( PhysicsShapeComponent* pComponent ) const;

        bool CreateCharacterActorAndShape( CharacterComponent* pComponent ) const;
        void DestroyCharacterActor( CharacterComponent* pComponent ) const;

        void UpdateStaticActorAndShape( PhysicsShapeComponent* pComponent ) const;
        void OnStaticShapeTransformUpdated( PhysicsShapeComponent* pComponent );

    private:

        PhysicsSystem*                                          m_pPhysicsSystem = nullptr;
        Scene*                                                  m_pScene = nullptr;

        TIDVector<ComponentID, CharacterComponent*>             m_characterComponents;
        TIDVector<ComponentID, PhysicsShapeComponent*>          m_physicsShapeComponents;
        TIDVector<ComponentID, PhysicsShapeComponent*>          m_dynamicShapeComponents; // TODO: profile and see if we need to use a dynamic pool

        EventBindingID                                          m_shapeTransformChangedBindingID;
        TVector<PhysicsShapeComponent*>                         m_staticActorShapeUpdateList;

        #if EE_DEVELOPMENT_TOOLS
        bool                                                    m_drawDynamicActorBounds = false;
        bool                                                    m_drawKinematicActorBounds = false;
        uint32_t                                                m_sceneDebugFlags = 0;
        float                                                   m_debugDrawDistance = 10.0f;
        #endif
    };
}