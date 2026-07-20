# HandleAllocator

Templated handle allocator for contiguous integer ranges.

Uses a 2-level 64-bit bitmask with per-page max-free-run hints. Allocations always pick the lowest available offset and try to maximize page utilization.

The L2 bitmask is the GPU-visible page data (`GetPageData()`). Handles carry their own allocation size; no separate query is needed for deallocation.

This allocator is purpose-built for GPU page tables, where offset minimization reduces dispatch overhead. For regular GPU memory allocation (buffers/textures) where offset doesn't affect GPU throughput, we still use D3D12MA.

## API

```cpp
template <typename OffsetType>  // uint16_t | uint32_t
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
| `GetPageData`        | Pointer to the L2 bitmask, directly uploadable to the GPU.                          |

## Design

### Data structures

| Level                  | Type                | Meaning                                                     |
| ---------------------- | ------------------- | ----------------------------------------------------------- |
| L1 — Page availability | `TVector<uint64_t>` | 1 bit per page. `1` = page has ≥1 free slot.                |
| L2 — Slot bitmask      | `TVector<uint64_t>` | 1 bit per slot. `1` = allocated. One word per 64-slot page. |
| Per-page hint          | `TVector<uint8_t>`  | Longest contiguous free run in the page (0–64).             |

Memory overhead at 1,024 pages (65K slots): ~9.2 KB.

### Why offset minimization matters

The L2 bitmask is uploaded to the GPU each frame. Compute shaders dispatch one thread per page (64 slots) and iterate the bits within that page. Each set bit represents a live element to process.

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

A TLSF-based allocator in MIN_TIME mode (no offset minimization) scatters allocations across pages. The LSB-first bitmask scan inherently produces tight packing — no strategy flag, no extra cost.

### Allocation strategy

- **N < 64:** LSB-first scan of L1. Per-page max-free-run hint skips fragmented pages without probing L2. Cross-page gaps detected by checking trailing-free of current page against leading-free of next page.
- **N = 64:** Multi-page scan. An empty page would suffice, but a cross-page gap at a lower offset takes priority.
- **N > 64:** Page-at-a-time scan using `_tzcnt_u64` to skip runs of allocated and free slots. Fully-free and fully-allocated pages are O(1) via the hint. Partially-full pages cost ~2–4 intrinsics per boundary instead of 64 bit tests.

Allocation failure only occurs when the pool hits the offset type's addressable limit (64K for `uint16_t`, 4G for `uint32_t`).

## Benchmarks vs D3D12MA `VirtualBlock`

All benchmarks use a fixed random seed and 131,072 slots. Factors are computed and printed by the test executable — re-run for exact values.

### Fragmentation stress

4,000 small allocs (1–15 slots), free every 3rd to create a fragmented baseline.
Then two timed phases, each measuring alloc + dealloc from that baseline:

1. 2,000 small allocs (1–15 slots) + free
2. 1,000 mixed-size allocs (1–100 slots, wide variance) + free

#### Debug

|              | HandleAllocator | D3D12MA MIN_OFFSET | D3D12MA MIN_TIME |
| ------------ | --------------- | ------------------ | ---------------- |
| Small allocs | 1.07 ms         | 12.70 ms           | 0.30 ms          |
| Large allocs | 1.82 ms         | 5.30 ms            | 0.15 ms          |
| Total        | 2.88 ms         | 17.99 ms           | 0.46 ms          |

#### Release

|              | HandleAllocator | D3D12MA MIN_OFFSET | D3D12MA MIN_TIME |
| ------------ | --------------- | ------------------ | ---------------- |
| Small allocs | 0.41 ms         | 7.41 ms            | 0.06 ms          |
| Large allocs | 0.61 ms         | 3.76 ms            | 0.03 ms          |
| Total        | 1.01 ms         | 11.16 ms           | 0.09 ms          |

### Offset quality

2,000 small allocs (1–80 slots), free every 3rd, then 300 mixed-size allocs (1–120 slots) into the fragmented pool.

|                    | Mean offset   | Max offset     | Packing ratio |
| ------------------ | ------------- | -------------- | ------------- |
| HandleAllocator    | 20,839        | 86,005         | 0.946         |
| D3D12MA MIN_OFFSET | 20,839        | 86,005         | 0.946         |
| D3D12MA MIN_TIME   | 66,330 (3.2×) | 86,391 (+0.4%) | 0.942 (−0.4%) |

Packing ratio = total allocated slots / max offset. Higher is tighter.

## Tradeoffs

**Cost: speed vs TLSF.** HandleAllocator is ~6.3× slower than D3D12MA MIN_TIME in Debug, ~11.1× in Release. The multi-page scan must traverse partially-full pages to find the lowest free offset. D3D12MA MIN_TIME uses a TLSF free-list with O(log n) lookup — an algorithmic advantage linear scanning cannot close. Fully-free and fully-allocated pages are skipped in O(1), but fragmented pages cost ~2–4 intrinsics each.

**Benefit: speed vs MIN_OFFSET.** HandleAllocator is ~6.2× faster than D3D12MA MIN_OFFSET in Debug, ~11.0× in Release. D3D12MA's TLSF pays a 39.3× (Debug) / 122.7× (Release) penalty for offset minimization compared to its own MIN_TIME mode. The LSB-first scan achieves equivalent packing at no extra cost — the offset quality table above confirms identical mean offset and packing ratio.
