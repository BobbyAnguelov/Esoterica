# HandleAllocator

The handle allocator hands out contiguous ranges of integer handles. It is purpose-built for GPU page tables: the leaf bitmask is uploaded directly to the GPU each frame, and tight packing minimizes the number of thread groups the GPU must process.

The pool is a hierarchical bitmask. Level 0 is the shader-facing slot bitmask — 64 slots per page, one bit per slot — with a stack of hint levels above it. Every page stores three derived values, collectively called its *hints*: `prefix` (free slots at the start), `suffix` (free slots at the end), and `maxRun` (longest contiguous free run). Allocations always pick the lowest available offset, which keeps pages as full as possible.

For regular GPU memory allocation (buffers and textures), where offset doesn't affect GPU throughput, we still use D3D12MA.

## Concepts

| Term                   | Meaning                                                                                                    |
| ---------------------- | ---------------------------------------------------------------------------------------------------------- |
| Slot / handle / offset | One integer index in the pool. The three words are used interchangeably.                                   |
| Page                   | 64 consecutive slots at level 0 — the unit of GPU work.                                                    |
| Level                  | One tier of the hierarchy. Level 0 is the shader-facing slot bitmask; level k groups 64 level-(k−1) pages. |
| Hint                   | A page's derived free-space values: `prefix`, `suffix`, `maxRun`.                                          |
| Dirty                  | A page whose hints no longer match its bitmask. Recomputed lazily before use.                              |

## API

```cpp
template <typename OffsetType, uint32_t NumHierarchyLevels>  // OffsetType: uint16_t | uint32_t
class HandleAllocator
{
public:
    struct Handle
    {
        OffsetType m_size   = OffsetType( 0 );
        OffsetType m_offset = ~OffsetType( 0 );

        bool IsValid() const;
    };

    void             Initialize( uint32_t initialCapacityInPages );
    void             Shutdown();

    Handle           Allocate( OffsetType numHandles );
    void             Deallocate( Handle&& handle );

    uint32_t         GetCapacityInPages() const;
    uint64_t const*  GetPageData() const;
};
```

| Name                 | Description                                                                         |
| -------------------- | ----------------------------------------------------------------------------------- |
| `Initialize`         | Reserve `initialCapacityInPages` pages (× 64 slots). Pool grows dynamically.        |
| `Shutdown`           | Assert no handles remain live, then free all resources.                             |
| `Allocate`           | Return a contiguous range at the lowest available offset. Debug asserts on failure. |
| `Deallocate`         | Free the range. Takes an rvalue reference — handle is invalidated.                  |
| `GetCapacityInPages` | Current page count.                                                                 |
| `GetPageData`        | Pointer to the level-0 bitmask, directly uploadable to the GPU.                     |

Handles carry their own allocation size, so `Deallocate` needs no extra parameters.

```cpp
HandleAllocator<uint32_t> allocator;
allocator.Initialize( 16 );

HandleAllocator<uint32_t>::Handle h = allocator.Allocate( 120 );

// ... upload allocator.GetPageData() to the GPU ...

allocator.Deallocate( std::move( h ) );
```

`NumHierarchyLevels` tunes the hint stack above the leaf bitmask (≥ 1). Deeper hierarchies skip more work per large-request scan and cost ~1% extra memory per level; levels are sized lazily so small pools only pay for the levels they use.

## Design

### Data structures

`HandleAllocator` holds `m_levels[NumHierarchyLevels]` of a per-level metadata struct:

| Field                           | Type                | Meaning                                                           |
| ------------------------------- | ------------------- | ----------------------------------------------------------------- |
| `m_slotMask`                    | `TVector<uint64_t>` | 64 slots per page, `1` = allocated. Level 0 only — shader-facing. |
| `m_availability`                | `TVector<uint64_t>` | 1 bit per 64 pages: page has ≥ 1 free slot. Level 0 fast path.    |
| `m_pagePrefix` / `m_pageSuffix` | `TVector<uint32_t>` | Free slots at the page start / end.                               |
| `m_pageMaxRun`                  | `TVector<uint32_t>` | Longest contiguous free run within the page.                      |
| `m_dirty`                       | `TVector<uint64_t>` | 1 bit per page: the page hints are stale (lazy maintenance).      |
- **Level 0** is the slot bitmask; one page = 64 slots.
- **Level k > 0** groups 64 pages of level k−1 into one page; a unit is 64^k slots, and the `{prefix, suffix, maxRun}` hints make the same scan code work at every level.

### Why offset minimization matters

Compute shaders dispatch one thread per page (64 slots) and iterate the bits within that page. Each set bit represents a live element to process.

A page with 1 live handle still dispatches 64 threads — 63 exit immediately. Tight packing reduces the number of page dispatches and increases the ratio of live threads per group.

```
Tight packing:                         Loose packing:
Page 0: ████████████████████████       Page 0: ██░░░░░░░░░░░░░░░░░░░░░░
Page 1: ████████████████████████       Page 1: ░░░░██░░░░░░░░░░░░░░░░░░
Page 2: ██████████████████░░░░░░       Page 2: ░░░░░░░░██░░░░░░░░░░░░░░
                                       Page 3: ░░░░░░░░░░░░████░░░░░░░░
  3 pages dispatched                     4 pages dispatched
  ~85% thread utilization                ~25% thread utilization
```

█ = allocated, ░ = free

A TLSF-based allocator (D3D12MA in MIN_TIME mode, which skips offset minimization) scatters allocations across pages. The LSB-first bitmask scan inherently produces tight packing without extra cost or additional flags.

### Allocation strategy

- **N < 64:** LSB-first scan of the level-0 availability map. The per-page max-free-run hint skips fragmented pages; cross-page gaps are detected via trailing-free of the current page vs leading-free of the next.
- **N ≥ 64:** hint-hierarchy scan from the top level. Pages whose `m_pageMaxRun` is smaller than the request are skipped in O(1); runs chaining across page boundaries are tracked via prefix/suffix carries, so cross-page gaps still win over later empty pages (exact lowest-offset semantics).

Allocation failure only occurs when the pool hits the offset type's addressable limit (64K for `uint16_t`, 4G for `uint32_t`).

### Maintenance

- Small alloc/free operations update only the touched leaf pages and set dirty bits — O(1) hot path.
- Hierarchy scans flush stale pages first: dirty pages are folded bottom-up, propagating to the parent level only when a page's folded value actually changed. The fold has a SIMD fast path for uniform pages (all children fully free / fully allocated — the common case for huge ranges).
- Structural changes (`Initialize`, grow, shrink) rebuild hints fully.
- `MarkAllocated`/`MarkFree` use 256-bit AVX2 stores for fully-covered interior pages: broadcast mask fill, hint zero/fill stores, and bulk availability-word updates. Partial first/last pages are handled scalar (bit-exact).

## Benchmarks vs D3D12MA `VirtualBlock`

All benchmarks use a fixed random seed and 131,072 slots. Performance numbers are printed by the test executable, rerun for exact results.

### Fragmentation stress

4,000 small allocs (1–15 slots), free every 3rd to create a fragmented baseline. Then two timed phases, each measuring alloc + dealloc from that baseline:

1. 2,000 small allocs (1–15 slots) + free
2. 1,000 mixed-size allocs (1–100 slots, wide variance) + free

#### Release

|              | HandleAllocator | D3D12MA MIN_OFFSET | D3D12MA MIN_TIME |
| ------------ | --------------- | ------------------ | ---------------- |
| Small allocs | 0.47 ms         | 7.55 ms            | 0.06 ms          |
| Large allocs | 0.55 ms         | 3.82 ms            | 0.03 ms          |
| Total        | 1.03 ms         | 11.37 ms           | 0.09 ms          |

#### Debug

|              | HandleAllocator | D3D12MA MIN_OFFSET | D3D12MA MIN_TIME |
| ------------ | --------------- | ------------------ | ---------------- |
| Small allocs | 1.10 ms         | 13.47 ms           | 0.30 ms          |
| Large allocs | 1.49 ms         | 5.52 ms            | 0.16 ms          |
| Total        | 2.60 ms         | 18.99 ms           | 0.46 ms          |

### Huge allocations into a fragmented pool

16M-slot pool, first 12M slots fragmented into alternating 8-free / 8-allocated runs (max free run = 8), 4M-slot "huge" allocations. Two phases: first the 4M-slot tail is free, so the alloc lands there; then the tail is occupied, so every allocator must scan the full pool and fail.

| Phase                                      | Debug        | Release      |
| ------------------------------------------ | ------------ | ------------ |
| Huge alloc lands in the free tail          | 0.2 ms/alloc | 0.1 ms/alloc |
| Huge alloc full scan + fail (tail blocked) | <1 µs/alloc  | <1 µs/alloc  |

The scan is O(skipped hierarchy pages) — the fail case is rejected by a single top-level max-run comparison. The tail-hit cost is the SIMD marking/freeing of the 4M slot bits plus the SIMD-accelerated hint flush.

### Offset quality

2,000 small allocs (1–80 slots), free every 3rd, then 300 mixed-size allocs (1–120 slots) into the fragmented pool.

|                    | Mean offset   | Max offset     | Packing ratio |
| ------------------ | ------------- | -------------- | ------------- |
| HandleAllocator    | 20,839        | 86,005         | 0.9           |
| D3D12MA MIN_OFFSET | 20,839        | 86,005         | 0.9           |
| D3D12MA MIN_TIME   | 66,330 (3.2×) | 86,391 (+0.4%) | 0.9 (−0.4%)   |

Packing ratio = total allocated slots / max offset. Higher is tighter.

## Tradeoffs

The two D3D12MA modes bracket this allocator: MIN_TIME is faster, MIN_OFFSET packs as tightly as we do. All multipliers below are derived from the fragmentation stress tables.

### vs D3D12MA MIN_TIME

TLSF has an algorithmic advantage — a free-list with O(log n) lookup — that a linear scan cannot close. We pay ~6.3× (Debug) / ~11.1× (Release) for offset minimization: the scan must traverse partially-full pages to find the lowest free offset. Fully-free and fully-allocated pages are skipped in O(1), but each fragmented page costs ~2–4 intrinsics.

### vs D3D12MA MIN_OFFSET

The win. We are ~6.2× faster in Debug and ~11.0× in Release, because D3D12MA's TLSF pays a 39.3× (Debug) / 122.7× (Release) penalty for minimizing offsets relative to its own MIN_TIME mode. The LSB-first scan gets equivalent packing for free — the offset quality table above shows identical mean offset and packing ratio.
