#include "WorldSystem_PlayerTargetingSystem.h"
#include "Game/Player/Components/Component_PlayerTargeting.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Hitbox/Components/Component_Hitbox.h"
#include "Engine/Camera/Components/Component_Camera.h"

//-------------------------------------------------------------------------

namespace EE
{
    void PlayerTargetingSystem::ShutdownSystem()
    {
        EE_ASSERT( m_targets.empty() );
        EE_ASSERT( m_trackers.empty() );
    }

    void PlayerTargetingSystem::RegisterComponent( Entity* pEntity, EntityComponent* pComponent )
    {
        if ( auto pHitboxComponent = TryCast<HitboxComponent>( pComponent ) )
        {
            m_targets.Add( TEntityComponentPair<HitboxComponent>( pEntity, pHitboxComponent ) );
        }
        else if ( auto pTrackerComponent = TryCast<PlayerTargetingComponent>( pComponent ) )
        {
            m_trackers.Add( TEntityComponentPair<PlayerTargetingComponent>( pEntity, pTrackerComponent ) );
        }
    }

    void PlayerTargetingSystem::UnregisterComponent( Entity* pEntity, EntityComponent* pComponent )
    {
        if ( auto pHitboxComponent = TryCast<HitboxComponent>( pComponent ) )
        {
            m_targets.Remove( pHitboxComponent->GetID() );
        }
        else if ( auto pTrackerComponent = TryCast<PlayerTargetingComponent>( pComponent ) )
        {
            m_trackers.Remove( pTrackerComponent->GetID() );
        }
    }

    void PlayerTargetingSystem::UpdateSystem( EntityWorldUpdateContext const& ctx )
    {
        if ( ctx.GetUpdateStage() == UpdateStage::GameSetup )
        {
            #if EE_DEVELOPMENT_TOOLS
            auto drawContext = ctx.GetDebugDrawContext();
            #endif

            // Update targets
            TInlineVector<HitboxComponent const*, 20> targets;
            for ( auto const& targetRecord : m_targets )
            {
                if ( targetRecord.m_pComponent->IsEnabled() && targetRecord.m_pComponent->HasHitbox() )
                {
                    Hitbox* pHitbox = targetRecord.m_pComponent->GetHitbox();
                    pHitbox->Update( ctx.GetScaledDeltaTime(), targetRecord.m_pEntity->GetRootSpatialComponent() );
                    targets.emplace_back( targetRecord.m_pComponent );

                    #if EE_DEVELOPMENT_TOOLS
                    //targetRecord.m_pComponent->GetHitbox()->DrawDebug( drawContext );
                    #endif
                }
            }

            // Reflect to trackers
            for ( auto const& trackerRecord : m_trackers )
            {
                auto pCamera = trackerRecord.m_pEntity->TryGetComponent<CameraComponent>();
                EE_ASSERT( pCamera != nullptr );

                Math::ViewVolume const& viewVolume = pCamera->GetViewVolume();
                trackerRecord.m_pComponent->ReflectTargets( ctx.GetScaledDeltaTime(), ctx.GetUnscaledDeltaTime(), viewVolume, targets );

                //-------------------------------------------------------------------------

                #if EE_DEVELOPMENT_TOOLS
                trackerRecord.m_pComponent->DrawDebug( drawContext );
                #endif
            }
        }
    }
}