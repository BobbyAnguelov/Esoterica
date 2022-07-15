#include "PhysicsQuery.h"
#include "Engine/Entity/EntityComponent.h"
#include <PxRigidActor.h>

//-------------------------------------------------------------------------

namespace EE::Physics
{
    physx::PxQueryHitType::Enum QueryFilter::preFilter( physx::PxFilterData const& filterData, physx::PxShape const* pShape, physx::PxRigidActor const* pActor, physx::PxHitFlags& queryFlags )
    {
        for ( auto const& ignoredComponentID : m_ignoredComponents )
        {
            auto pOwnerComponent = reinterpret_cast<EntityComponent const*>( pActor->userData );
            if ( pOwnerComponent->GetID() == ignoredComponentID )
            {
                return physx::PxQueryHitType::eNONE;
            }
        }

        //-------------------------------------------------------------------------

        for ( auto const& ignoredEntityID : m_ignoredEntities )
        {
            auto pOwnerComponent = reinterpret_cast<EntityComponent const*>( pActor->userData );
            if ( pOwnerComponent->GetEntityID() == ignoredEntityID )
            {
                return physx::PxQueryHitType::eNONE;
            }
        }

        //-------------------------------------------------------------------------

        return physx::PxQueryHitType::eBLOCK;
    }

    physx::PxQueryHitType::Enum QueryFilter::postFilter( physx::PxFilterData const& filterData, physx::PxQueryHit const& hit )
    {
        EE_UNREACHABLE_CODE(); // Not currently used
        return physx::PxQueryHitType::eBLOCK;
    }
}