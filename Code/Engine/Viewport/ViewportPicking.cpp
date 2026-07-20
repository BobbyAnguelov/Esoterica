
#include "ViewportPicking.h"
#include "Base/Math/Math.h"
#include "Base/Profiling.h"
#include "EASTL/sort.h"
//-------------------------------------------------------------------------

namespace EE
{
    #if EE_DEVELOPMENT_TOOLS
    void PickingData::DeduplicateAndSort()
    {
        EE_PROFILE_FUNCTION_RENDER();

        for ( auto first = begin(); first < end(); ++first )
        {
            for ( auto second = first + 1; second < end(); ++second )
            {
                // If we have duplicate IDs keep the one with the lowest distance
                if ( *first == *second )
                {
                    first->m_distance = Math::Min( first->m_distance, second->m_distance );
                    VectorEraseUnordered( *this, second-- );
                }
            }
        }

        auto SortPredicate = [] ( PickingID const& a, PickingID const& b )
        {
            auto ConstructSortingKey = [] ( PickingID const& id )
            {
                uint32_t distance = 0;
                memcpy( &distance, &id.m_distance, sizeof( float ) );

                return uint64_t( id.m_sortPriority ) << 32 | uint64_t( distance );
            };

            uint64_t sortingKey0 = ConstructSortingKey( a );
            uint64_t sortingKey1 = ConstructSortingKey( b );

            return sortingKey0 < sortingKey1;
        };

        eastl::sort( begin(), end(), SortPredicate );
    }
    #endif
}
