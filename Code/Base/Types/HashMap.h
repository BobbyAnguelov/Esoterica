#pragma once

#include "Base/ThirdParty/EA/eastl_Esoterica_TrackedAllocator.h"
#include <EASTL/hash_map.h>

//-------------------------------------------------------------------------

namespace EE
{
    template<typename K, typename V> using THashMap = eastl::hash_map<K, V, eastl::hash<K>, eastl::equal_to<K>, eastl::TrackedAllocator, false>;
    template<typename K, typename V> using TPair = eastl::pair<K, V>;
}