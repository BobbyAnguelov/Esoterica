#pragma once

#include "Game/Player/PlayerInputState.h"
#include "Game/Player/Components/Component_Player.h"
#include "Game/Player/Animation/PlayerAnimationController.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"

//-------------------------------------------------------------------------
// The context for all player actions
//-------------------------------------------------------------------------
// Provides the common set of systems and components needed for player actions

namespace EE
{
    class EntityComponent;
    class PlayerCameraComponent;
    namespace Input { class InputSystem; }

    //-------------------------------------------------------------------------

    struct PlayerActionContext
    {
        ~PlayerActionContext();

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

        EE_FORCE_INLINE Seconds GetDeltaTime() const { return m_pEntityWorldUpdateContext->GetScaledDeltaTime(); }
        EE_FORCE_INLINE Seconds GetScaledDeltaTime() const { return m_pEntityWorldUpdateContext->GetScaledDeltaTime(); }
        EE_FORCE_INLINE Seconds GetRawDeltaTime() const { return m_pEntityWorldUpdateContext->GetRawDeltaTime(); }
        template<typename T> inline T* GetWorldSystem() const { return m_pEntityWorldUpdateContext->GetWorldSystem<T>(); }
        template<typename T> inline T* GetSystem() const { return m_pEntityWorldUpdateContext->GetSystem<T>(); }

        #if EE_DEVELOPMENT_TOOLS
        DebugDrawContext GetDebugDrawContext() const;
        #endif

    public:

        EntityWorldUpdateContext const*             m_pEntityWorldUpdateContext = nullptr;
        PlayerInputState*                           m_pInput = nullptr;
        Physics::PhysicsWorld*                      m_pPhysicsWorld = nullptr;

        PlayerComponent*                            m_pPlayer = nullptr;
        PlayerGameState*                            m_pPlayerState = nullptr;
        PlayerCameraComponent*                      m_pCamera = nullptr;
        PlayerAnimationController*                  m_pAnimationController = nullptr;
        TInlineVector<EntityComponent*, 10>         m_components;
    };
}