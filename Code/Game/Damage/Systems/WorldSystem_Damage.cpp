#include "WorldSystem_Damage.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Hitbox/Components/Component_Hitbox.h"
#include "Game/Damage/Components/Component_Damage.h"
#include "Game/Damage/Components/Component_Health.h"

//-------------------------------------------------------------------------

namespace EE
{
    void DamageSystem::ShutdownSystem()
    {
        EE_ASSERT( m_dealers.empty() );
        EE_ASSERT( m_receivers.empty() );
    }

    //-------------------------------------------------------------------------

    void DamageSystem::RegisterComponent( Entity* pEntity, EntityComponent* pComponent )
    {
        if ( auto pDamageComponent = TryCast<DamageComponent>( pComponent ) )
        {
            m_dealers.Add( TEntityComponentPair<DamageComponent>( pEntity, pDamageComponent ) );
        }
        else if ( auto pHitboxComponent = TryCast<HitboxComponent>( pComponent ) )
        {
            m_receivers.Add( TEntityComponentPair<HitboxComponent>( pEntity, pHitboxComponent ) );
        }
    }

    void DamageSystem::UnregisterComponent( Entity* pEntity, EntityComponent* pComponent )
    {
        if ( auto pDamageComponent = TryCast<DamageComponent>( pComponent ) )
        {
            m_dealers.Remove( pDamageComponent->GetID() );
        }
        else if ( auto pHitboxComponent = TryCast<HitboxComponent>( pComponent ) )
        {
            m_receivers.Remove( pHitboxComponent->GetID() );
        }
    }

    //-------------------------------------------------------------------------

    void DamageSystem::UpdateSystem( EntityWorldUpdateContext const& ctx )
    {
        if ( !ctx.IsGameWorld() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        TInlineVector<Hitbox::Hit, 10> hits;

        for ( auto& dealer : m_dealers )
        {
            for ( DamageComponent::Request& dmg : dealer.m_pComponent->m_requests )
            {
                for ( auto& receiver : m_receivers )
                {
                    Hitbox const* pHitBox = receiver.m_pComponent->GetHitbox();
                    if ( !receiver.m_pComponent->IsEnabled() )
                    {
                        continue;
                    }

                    if ( pHitBox->CollideRay( dmg.m_start, dmg.m_end, hits ) )
                    {
                        auto pHealthComponent = receiver.m_pEntity->TryGetComponent<HealthComponent>();
                        if ( pHealthComponent != nullptr )
                        {
                            float dmgMultiplier = 1.0f;

                            switch ( hits.front().m_pShape->m_severity )
                            {
                                case HitboxDamageSeverity::Critical:
                                dmgMultiplier = 8.0f;
                                break;

                                case HitboxDamageSeverity::High:
                                dmgMultiplier = 4.5f;
                                break;

                                case HitboxDamageSeverity::Medium:
                                dmgMultiplier = 2.5f;
                                break;

                                case HitboxDamageSeverity::Low:
                                dmgMultiplier = 1.0f;
                                break;
                            }

                            pHealthComponent->SetHP( pHealthComponent->GetHP() - ( dmg.m_info.m_hitpoints * dmgMultiplier ) );
                        }
                    }
                }
            }

            dealer.m_pComponent->m_requests.clear();
        }

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        auto drawCtx = ctx.GetDebugDrawContext();
        for ( auto& receiver : m_receivers )
        {
            Hitbox const* pHitBox = receiver.m_pComponent->GetHitbox();
            if ( receiver.m_pComponent->IsEnabled() )
            {
                //pHitBox->DrawDebug( drawCtx );
            }

            auto pHealthComponent = receiver.m_pEntity->TryGetComponent<HealthComponent>();
            if ( pHealthComponent != nullptr )
            {
                InlineString str( InlineString::CtorSprintf(), "HP: %.0f", pHealthComponent->GetHP() );
                Color color = Color::EvaluateRedGreenGradient( pHealthComponent->GetHP() / 100, true );
                drawCtx.DrawText3D( receiver.m_pEntity->GetWorldTransform().GetTranslation(), str.c_str(), color );
            }
        }
        #endif
    }
}