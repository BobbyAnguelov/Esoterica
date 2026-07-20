#include "Behavior_Death.h"
#include "Engine/Hitbox/Components/Component_Hitbox.h"

//-------------------------------------------------------------------------

namespace EE
{
    void DeathBehavior::StartInternal( BehaviorContext const& ctx )
    {
        auto pHitboxComponent = ctx.GetComponentByType<HitboxComponent>();
        pHitboxComponent->SetEnabled( false );
    }

    Behavior::Status DeathBehavior::UpdateInternal( BehaviorContext const& ctx )
    {
        m_deathAction.Update( ctx );
        return Status::Running;
    }

    void DeathBehavior::StopInternal( BehaviorContext const& ctx, StopReason reason )
    {}
}