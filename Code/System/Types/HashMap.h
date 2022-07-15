#pragma once

#include "System/Memory/Memory.h"
#include <EASTL/hash_map.h>

//-------------------------------------------------------------------------

namespace EE
{
    template<typename K, typename V> using THashMap = eastl::hash_map<K, V, eastl::hash<K>>;
    template<typename K, typename V> using TPair = eastl::pair<K, V>;
}