#pragma once
#include "Base/Types/Arrays.h"
#include "Engine/_Module/API.h"

//-------------------------------------------------------------------------

namespace EE
{
    #if EE_DEVELOPMENT_TOOLS
    struct alignas( 8 ) PickingResult
    {
        uint32_t                    m_instanceID;
        float                       m_intersectionDistance;
        uint32_t                    m_sortPriority;
        uint32_t                    m_padding0;
        uint64_t                    m_hitTestID;
        uint64_t                    m_padding1;
    };

    struct PickingID
    {
        constexpr static uint64_t const InvalidID = UINT64_MAX;

    public:

        PickingID() = default;

        PickingID( uint64_t primaryID, uint64_t secondaryID, uint32_t sortPriority, float distance )
            : m_primaryID( primaryID )
            , m_secondaryID( secondaryID )
            , m_sortPriority( sortPriority )
            , m_distance( distance )
        {}

        inline void Reset() { m_primaryID = m_secondaryID = InvalidID; m_distance = 0.0f; }
        inline bool IsSet() const { return m_primaryID != InvalidID; }
        inline bool HasSecondaryID() const { return m_secondaryID != InvalidID; }

        inline bool operator==( PickingID const& rhs ) const
        {
            return m_primaryID == rhs.m_primaryID && m_secondaryID == rhs.m_secondaryID;
        }

    public:

        uint64_t    m_primaryID = InvalidID;
        uint64_t    m_secondaryID = InvalidID;
        uint32_t    m_sortPriority = ~0U;
        float       m_distance = 0.0f;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINE_API PickingData : public TVector<PickingID>
    {
        void DeduplicateAndSort();
    };
    #endif
}
