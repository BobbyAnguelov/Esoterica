#include "PhysicsQuery.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    void QueryRules::Reset()
    {
        m_collidesWithMask = 0;
        m_ignoredComponents.clear();
        m_ignoredEntities.clear();
        m_allowMultipleHits = false;

        UpdateInternals();
    }

    void QueryRules::UpdateInternals()
    {
        // Hit Flags - Only relevant for casts/sweeps
        //-------------------------------------------------------------------------

        m_hitFlags = physx::PxHitFlag::eDEFAULT | physx::PxHitFlag::eMTD;

        // Filter Data
        //-------------------------------------------------------------------------

        m_queryFilterData.data.word0 = m_collidesWithMask; // Word 0 is the collision mask for queries
        m_queryFilterData.flags = physx::PxQueryFlag::ePREFILTER;

        switch ( m_mobilityFilter )
        {
            case StaticAndDynamic:
            m_queryFilterData.flags |= ( physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::eDYNAMIC );
            break;

            case OnlyStatic:
            m_queryFilterData.flags |= physx::PxQueryFlag::eSTATIC;
            break;

            case OnlyDynamic:
            m_queryFilterData.flags |= physx::PxQueryFlag::eDYNAMIC;
            break;
        }
    }
}