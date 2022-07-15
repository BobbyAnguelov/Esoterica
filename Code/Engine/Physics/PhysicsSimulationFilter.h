#pragma once

#include "PhysX.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class SimulationFilter : public physx::PxSimulationFilterCallback
    {
    public:

        static physx::PxFilterFlags Shader( physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0, physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1, physx::PxPairFlags& pairFlags, void const* constantBlock, uint32_t constantBlockSize );
        virtual ~SimulationFilter() = default;

    private:

        virtual physx::PxFilterFlags pairFound( physx::PxU32 pairID, physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0, const physx::PxActor* a0, const physx::PxShape* s0, physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1, const physx::PxActor* a1, const physx::PxShape* s1, physx::PxPairFlags& pairFlags ) override;
        virtual void pairLost( physx::PxU32 pairID, physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0, physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1, bool objectRemoved ) override;
        virtual bool statusChange( physx::PxU32& pairID, physx::PxPairFlags& pairFlags, physx::PxFilterFlags& filterFlags ) override;
    };
}