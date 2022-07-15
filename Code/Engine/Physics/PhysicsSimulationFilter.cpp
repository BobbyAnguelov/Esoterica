#include "PhysicsSimulationFilter.h"
#include "PhysicsRagdoll.h"

//-------------------------------------------------------------------------

using namespace physx;

//-------------------------------------------------------------------------

namespace EE::Physics
{
    PxFilterFlags SimulationFilter::Shader( PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, PxPairFlags& pairFlags, void const* constantBlock, uint32_t constantBlockSize )
    {
        // Triggers
        //-------------------------------------------------------------------------

        if ( PxFilterObjectIsTrigger( attributes0 ) || PxFilterObjectIsTrigger( attributes1 ) )
        {
            pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
            return PxFilterFlag::eDEFAULT;
        }

        // Articulations
        //-------------------------------------------------------------------------

        if ( PxGetFilterObjectType( attributes0 ) == PxFilterObjectType::eARTICULATION && PxGetFilterObjectType( attributes1 ) == PxFilterObjectType::eARTICULATION )
        {
            pairFlags = PxPairFlag::eCONTACT_DEFAULT;
            return PxFilterFlag::eCALLBACK;
        }

        pairFlags = PxPairFlag::eCONTACT_DEFAULT;
        return PxFilterFlag::eDEFAULT;
    }

    physx::PxFilterFlags SimulationFilter::pairFound( PxU32 pairID, PxFilterObjectAttributes attributes0, PxFilterData filterData0, const PxActor* a0, const PxShape* s0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, const PxActor* a1, const PxShape* s1, PxPairFlags& pairFlags )
    {
        // Articulations
        //-------------------------------------------------------------------------

        if ( PxGetFilterObjectType( attributes0 ) == PxFilterObjectType::eARTICULATION && PxGetFilterObjectType( attributes1 ) == PxFilterObjectType::eARTICULATION )
        {
            auto pL0 = static_cast<PxArticulationLink const*>( a0 );
            auto pL1 = static_cast<PxArticulationLink const*>( a1 );
            PxArticulationBase const* pArticulation0 = &pL0->getArticulation();
            PxArticulationBase const* pArticulation1 = &pL1->getArticulation();
            Ragdoll const* pRagdoll0 = reinterpret_cast<Ragdoll*>( pArticulation0->userData );
            Ragdoll const* pRagdoll1 = reinterpret_cast<Ragdoll*>( pArticulation1->userData );

            // If these are different articulation on the same user, then ignore collisions
            if ( pArticulation0 != pArticulation1 && pRagdoll0->GetUserID() == pRagdoll1->GetUserID() )
            {
                return PxFilterFlag::eKILL;
            }

            // If these are the same articulation, then check the self-collision rules
            if ( pArticulation0 == pArticulation1 )
            {
                uint64_t const bodyIdx0 = (uint64_t) pL0->userData;
                uint64_t const bodyIdx1 = (uint64_t) pL1->userData;
                if ( !pRagdoll0->ShouldBodiesCollides( (int32_t) bodyIdx0, (int32_t) bodyIdx1 ) )
                {
                    return PxFilterFlag::eKILL;
                }
            }
        }

        return PxFilterFlag::eDEFAULT;
    }

    void SimulationFilter::pairLost( PxU32 pairID, PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, bool objectRemoved )
    {
        // Do Nothing
    }

    bool SimulationFilter::statusChange( PxU32& pairID, PxPairFlags& pairFlags, PxFilterFlags& filterFlags )
    {
        return false;
    }
}