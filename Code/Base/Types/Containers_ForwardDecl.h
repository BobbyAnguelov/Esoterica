#pragma once
#include "Base/Esoterica.h"

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

    template <typename T1, typename T2>
    struct pair;

    template <typename T, size_t Extent>
    class span;

    //-------------------------------------------------------------------------

    template <size_t Alignment> class TrackedAllocatorBase;
    using TrackedAllocator = TrackedAllocatorBase<16>;
    using TrackedAlignedAllocator = TrackedAllocatorBase<32>;
}

//-------------------------------------------------------------------------

namespace EE
{
    using String = eastl::basic_string<char, eastl::TrackedAllocator>;
    template<size_t S> using TInlineString = eastl::fixed_string<char, S, true, eastl::TrackedAllocator>;
    using InlineString = eastl::fixed_string<char, 255, true, eastl::TrackedAllocator>;

    template<typename T> using TVector = eastl::vector<T, eastl::TrackedAllocator>;
    template<typename T, size_t S> using TInlineVector = eastl::fixed_vector<T, S, true, eastl::TrackedAllocator>;
    template<typename T, size_t S> using TArray = eastl::array<T, S>;

    template <typename T> using TAlignedVector = eastl::vector<T, eastl::TrackedAlignedAllocator>;

    using Blob = TVector<uint8_t>;

    template <typename T> using TArrayView = eastl::span<T, size_t( -1 )>;

    template<typename K, typename V> using THashMap = eastl::hash_map<K, V, eastl::hash<K>, eastl::equal_to<K>, eastl::TrackedAllocator, false>;

    template<typename K, typename V> using TPair = eastl::pair<K, V>;
}
