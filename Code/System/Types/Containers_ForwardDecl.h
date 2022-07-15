#pragma once
#include "System/Esoterica.h"

//-------------------------------------------------------------------------

namespace eastl
{
    class allocator;

    template <typename T>
    struct equal_to;

    template <typename T>
    struct hash;

    template <typename T, typename Allocator>
    class basic_string;

    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    class fixed_string;

    template <typename T, typename Allocator>
    class vector;

    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    class fixed_vector;

    template <typename T, size_t N>
    struct array;

    template <typename Key, typename T, typename Hash, typename Predicate, typename Allocator, bool bCacheHashCode>
    class hash_map;
}

//-------------------------------------------------------------------------

namespace EE
{
    using String = eastl::basic_string<char, eastl::allocator>;
    template<size_t S> using TInlineString = eastl::fixed_string<char, S, true, eastl::allocator>;
    using InlineString = eastl::fixed_string<char, 255, true, eastl::allocator>;

    template<typename T> using TVector = eastl::vector<T, eastl::allocator>;
    template<typename T, size_t S> using TInlineVector = eastl::fixed_vector<T, S, true, eastl::allocator>;
    template<typename T, size_t S> using TArray = eastl::array<T, S>;

    using Blob = TVector<uint8_t>;

    template<typename K, typename V> using THashMap = eastl::hash_map<K, V, eastl::hash<K>, eastl::equal_to<K>, eastl::allocator, false>;
}