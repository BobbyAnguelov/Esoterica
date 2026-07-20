rapidhash - Very fast, high quality, platform-independent
====

Family of three hash functions: rapidhash, rapidhashMicro and rapidhashNano

Used by [Chromium](https://chromium-review.googlesource.com/c/chromium/src/+/5667077), [Folly's F14](https://github.com/facebook/folly/blob/main/folly/container/HeterogeneousAccess.h#L130), [Ninja](https://github.com/ninja-build/ninja/blob/656412538b6fc102b809a61e0efce422e5a20534/src/build_log.cc#L61), [JuliaLang](https://github.com/JuliaLang/julia/blob/309b1b158f59485772d5f5fe0a762f20185cf799/base/hashing.jl#L281), [ziglang](https://github.com/ziglang/zig/pull/22085), [fb303](https://github.com/facebook/fb303/blob/6c0666c96b7112be2cb5608385063f1aed19c43f/fb303/ThreadCachedServiceData.h#L461), among others

**Rapidhash**  
General purpose hash function, amazing performance across all sizes.  
Surpasses [70GB/s](https://github.com/Nicoshev/rapidhash/tree/master?tab=readme-ov-file#outstanding-performance) on Apple's M4 cpus.  
Clang-18+ compiles it to ~185 instructions, both on x86-64 and aarch64.  
The fastest recommended hash function by [SMHasher](https://github.com/rurban/smhasher?tab=readme-ov-file#summary) and [SMHasher3](https://gitlab.com/fwojcik/smhasher3/-/blob/main/results/README.md#passing-hashes).

**RapidhashMicro**  
Designed for HPC and server applications, where cache misses make a noticeable performance detriment.  
Clang-18+ compiles it to ~140 instructions without stack usage, both on x86-64 and aarch64.  
Faster for sizes up to 512 bytes, just 15%-20% slower for inputs above 1kb.  
Produces same output as Rapidhash for inputs up to 80 bytes.

**RapidhashNano**  
Designed for Mobile and embedded applications, where keeping a small code size is a top priority.  
Clang-18+ compiles it to less than 100 instructions without stack usage, both on x86-64 and aarch64.  
The fastest for sizes up to 48 bytes, but may be considerably slower for larger inputs.  
Produces same output as Rapidhash for inputs up to 48 bytes.

**Streamable**  
The three functions can be computed without knowing the input length upfront.

**Universal**  
All functions have been optimized for both AMD64 and AArch64 systems.  
Compatible with gcc, clang, icx and MSVC.  
They do not use machine-specific vectorized or cryptographic instruction sets.

**Excellent**  
All functions pass all tests in both [SMHasher](https://github.com/rurban/smhasher/blob/master/doc/rapidhash.txt) and [SMHasher3](https://gitlab.com/fwojcik/smhasher3/-/blob/main/results/raw/rapidhash.txt).  
[Collision-based study](https://github.com/Nicoshev/rapidhash/tree/master?tab=readme-ov-file#collision-based-hash-quality-study) showed a collision probability close to ideal.  
Outstanding collision ratio when tested with datasets of 16B and 67B keys:

| Input Len | Nb Hashes | Expected | Nb Collisions |
| ---  | ---   | ---   | --- |
|   12 | 15 Gi |   7.0 |   6 |
|   16 | 15 Gi |   7.0 |   7 |
|   24 | 15 Gi |   7.0 |   7 |
|   32 | 15 Gi |   7.0 |  10 |
|   40 | 15 Gi |   7.0 |   4 |
|   48 | 15 Gi |   7.0 |   7 |
|   64 | 15 Gi |   7.0 |   6 |
|   80 | 15 Gi |   7.0 |  11 |
|   96 | 15 Gi |   7.0 |   6 |
|  120 | 15 Gi |   7.0 |   8 |
|  128 | 15 Gi |   7.0 |   6 |
|   12 | 62 Gi | 120.1 | 122 |
|   16 | 62 Gi | 120.1 |  97 |
|   24 | 62 Gi | 120.1 | 125 |
|   32 | 62 Gi | 120.1 | 131 |
|   40 | 62 Gi | 120.1 | 117 |
|   48 | 62 Gi | 120.1 | 146 |
|   64 | 62 Gi | 120.1 | 162 |
|   80 | 62 Gi | 120.1 | 165 |
|   96 | 62 Gi | 120.1 | 180 |
|  120 | 62 Gi | 120.1 | 168 |

More results can be found in the [collisions folder](https://github.com/Nicoshev/rapidhash/tree/master/collisions)

Outstanding performance
-------------------------

Average latency when hashing keys of 4, 8 and 16 bytes

| Hash      | M1 Pro | M3 Pro | Neoverse V2 | AMD Turin | Ryzen 9700X |
| ---       | ---    | ---    | ---         | ---       | ---         |
| rapidhash | 1.79ns | 1.38ns | 2.05ns      | 2.31ns    | 1.46ns      |
| xxh3      | 1.92ns | 1.50ns | 2.15ns      | 2.35ns    | 1.45ns      |

Peak throughput when hashing files of 16Kb-2Mb

| Hash      | M1 Pro | M3 Pro | M3 Ultra | M4     | Neoverse V2 | Ryzen 9700X |
| ---       | ---    | ---    | ---      | ---    | ---         | ---         |
| rapidhash | 47GB/s | 57GB/s | 61GB/s   | 71GB/s | 38GB/s      | 68GB/s      |
| xxh3      | 37GB/s | 43GB/s | 47GB/s   | 49GB/s | 34GB/s      | 78GB/s      |

Long-input measurements were taken compiling with the RAPIDHASH_UNROLLED macro.

The benchmarking program can be found in the [bench folder](https://github.com/Nicoshev/rapidhash/tree/master/bench)

Collision-based hash quality study
-------------------------

A perfect hash function distributes its domain uniformly onto the image.  
When the domain's cardinality is a multiple of the image's cardinality, each potential output has the same probability of being produced.  
A function producing 64-bit hashes should have a $p=1/2^{64}$ of generating each output.

If we compute $n$ hashes, the expected amount of collisions should be the number of unique input pairs times the probability of producing a given hash.  
This should be $(n*(n-1))/2 * 1/2^{64}$, or simplified: $(n*(n-1))/2^{65}$.  
In the case of hashing $15*2^{30}$ (~16.1B) different keys, we should expect to see $7.03$ collisions.

We present an experiment in which we use rapidhash to hash $68$ datasets of $15*2^{30}$ (15Gi) keys each.  
For each dataset, the amount of collisions produced is recorded as measurement.  
Ideally, the average among measurements should be $7.03$ and the results collection should be a binomial distribution.  
We obtained a mean value of $7.60$, just $8.11$% over $7.03$.  
Each dataset individual result and the collisions test program can be found in the [collisions folder](https://github.com/Nicoshev/rapidhash/tree/master/collisions).  
The default seed $0$ was used in all experiments.

Ports
-------------------------
[Java](https://github.com/dynatrace-oss/hash4j?tab=readme-ov-file#hash-algorithms) by hash4j  
[Rust](https://github.com/hoxxep/rapidhash) by hoxxep    
