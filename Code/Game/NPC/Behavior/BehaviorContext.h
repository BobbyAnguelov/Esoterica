#pragma once

#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Game/NPC/Components/Component_NPC.h"
#include "Game/NPC/NPCGameState.h"
#include "Game/NPC/Animation/NPCAnimationController.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EntityComponent;
    namespace Navmesh { class NavmeshWorldSystem; }

    //-------------------------------------------------------------------------
    // The context for all NPC behaviors
    //-------------------------------------------------------------------------
    // Provides the common set of systems and components needed for AI behaviors/actions

    struct BehaviorContext
    {
        ~BehaviorContext();

        bool IsValid() const;

        template<typename T>
        T* GetComponentByType() const
        {
            static_assert( std::is_base_of<EntityComponent, T>::value, "T must be a component type" );
            for ( auto pComponent : m_components )
            {
                if ( auto pCastComponent = TryCast<T>( pComponent ) )
                {
                    return pCastComponent;
                }
            }

            return nullptr;
        }

        // Forwarding helper functions
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE Seconds GetDeltaTime() const { return m_pEntityWorldUpdateContext->GetDeltaTime(); }
        template<typename T> inline T* GetWorldSystem() const { return m_pEntityWorldUpdateContext->GetWorldSystem<T>(); }
        template<typename T> inline T* GetSystem() const { return m_pEntityWorldUpdateContext->GetSystem<T>(); }

        #if EE_DEVELOPMENT_TOOLS
        DebugDrawContext GetDebugDrawContext() const;
        #endif

    public:

        EntityWorldUpdateContext const*             m_pEntityWorldUpdateContext = nullptr;
        Physics::PhysicsWorld*                      m_pPhysicsWorld = nullptr;
        Navmesh::NavmeshWorldSystem*                m_pNavmeshSystem = nullptr;

        NPCComponent*                               m_pNPC = nullptr;
        NPCGameState*                               m_pNPCState = nullptr;
        NPCAnimationController*                     m_pAnimationController = nullptr;
        TInlineVector<EntityComponent*, 10>         m_components;
    };
}