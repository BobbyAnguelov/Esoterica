#pragma once

#include "Game/_Module/API.h"
#include "Game/Damage/Damage.h"
#include "Engine/Entity/EntityComponent.h"
#include "Base/Math/Vector.h"

//-------------------------------------------------------------------------

namespace EE
{
    struct DamageInfo;

    //-------------------------------------------------------------------------

    class EE_GAME_API DamageComponent : public EntityComponent
    {
        EE_TRANSIENT_ENTITY_COMPONENT( DamageComponent );

        friend class DamageSystem;

        struct Request
        {
            Vector          m_start;
            Vector          m_end;
            DamageInfo      m_info;
        };

    public:

        void RequestDamage( Vector const& start, Vector const& end, DamageInfo const& info )
        {
            m_requests.emplace_back( start, end, info );
        }

    private:

        TVector<Request> m_requests;
    };
}