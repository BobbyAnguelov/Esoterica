# LZAV - Fast Data Compression Algorithm (in C/C++)

## Introduction

LZAV is a fast general-purpose in-memory data compression algorithm based on
now-classic [LZ77](https://wikipedia.org/wiki/LZ77_and_LZ78) lossless data
compression method. LZAV holds a good position on the Pareto landscape of
factors, among many similar in-memory (non-streaming) compression algorithms.

The LZAV algorithm's code is portable, cross-platform, scalar, header-only,
and inlinable C (compatible with C++). It supports big- and little-endian
platforms, and any memory alignment models. The algorithm is efficient on both
32- and 64-bit platforms. Incompressible data almost does not expand.
Compliant with WebAssembly (WASI libc), and runs there at just twice lower
performance than the native code.

LZAV does not sacrifice internal out-of-bounds (OOB) checks for decompression
speed. This means that LZAV can be used in strict conditions where OOB memory
writes (and especially reads) that lead to a trap are unacceptable (e.g.,
real-time, system, server software). LZAV can be used safely (causing no
crashing nor UB) even when decompressing malformed or damaged compressed data,
which means that LZAV does not require calculation of a checksum (or hash) of
the compressed data. Only a checksum of the uncompressed data may be required,
depending on an application's guarantees.

The internal functions available in the `lzav.h` file allow you to easily
implement, and experiment with, your own compression algorithms. LZAV stream
format and decompressor have a potential of high decompression speeds and
compression ratios, which depend on the way data is compressed.

## Usage

To compress data:

```c
#include "lzav.h"

int max_len = lzav_compress_bound( src_len );
void* comp_buf = malloc( max_len );
int comp_len = lzav_compress_default( src_buf, comp_buf, src_len, max_len );

if( comp_len == 0 && src_len != 0 )
{
    // Error handling.
}
```

To decompress data:

```c
#include "lzav.h"

void* decomp_buf = malloc( src_len );
int l = lzav_decompress( comp_buf, decomp_buf, comp_len, src_len );

if( l < 0 )
{
    // Error handling.
}
```

To compress data with a higher ratio, for non-time-critical uses (e.g.,
compression of application's static assets):

```c
#include "lzav.h"

int max_len = lzav_compress_bound_hi( src_len ); // Note another bound function!
void* comp_buf = malloc( max_len );
int comp_len = lzav_compress_hi( src_buf, comp_buf, src_len, max_len );

if( comp_len == 0 && src_len != 0 )
{
    // Error handling.
}
```

LZAV algorithm and the source code (which conforms to
[ISO C99](https://en.wikipedia.org/wiki/C99)) were quality-tested on:
Clang, GCC, MSVC, and Intel C++ compilers; on x86, x86-64 (Intel, AMD),
and AArch64 (Apple Silicon) architectures; Windows 10, AlmaLinux 9.3, and
macOS 15.7. Full C++ compliance is enabled conditionally and automatically
when the source code is compiled with a C++ compiler.

## Ports

* [C++, vcpkg](https://vcpkg.link/ports/lzav)
* [FreeArc, by Shegorat](https://krinkels.org/resources/cls-lzav.579/)
* [Rust, by pkolaczk](https://crates.io/crates/lzav)

## Customizing C++ namespace

In C++ environments where it is undesirable to export LZAV symbols into the
global namespace, the `LZAV_NS_CUSTOM` macro can be defined externally:

```c++
#define LZAV_NS_CUSTOM lzav
#include "lzav.h"
```

Similarly, LZAV symbols can be placed into any other custom namespace (e.g.,
a namespace with data compression functions):

```c++
#define LZAV_NS_CUSTOM my_namespace
#include "lzav.h"
```

This way, LZAV symbols and functions can be referenced like
`my_namespace::lzav_compress_default(...)`. Note that since all LZAV functions
have the `static` specifier, there can be no ABI conflicts, even if the LZAV
header is included in unrelated, mixed C/C++, compilation units.

## Comparisons

The tables below present performance ballpark numbers of LZAV algorithm
(based on Silesia dataset).

While LZ4 seems to compress faster, LZAV comparably provides 15.5% memory
storage cost savings. This is a significant benefit in database and file
system use cases since compression is only about 35% slower while CPUs rarely
run at their maximum capacity anyway (considering cached data writes are
deferred in background threads), and disk I/O times are reduced due to a
better compression. In general, LZAV holds a very strong position in this
class of data compression algorithms, if one considers all factors:
compression and decompression speeds, compression ratio, and just as
important - code maintainability: LZAV is maximally portable and has a rather
small independent codebase.

Performance of LZAV is not limited to the presented ballpark numbers.
Depending on the data being compressed, LZAV can achieve 800 MB/s compression
and 5000 MB/s decompression speeds. Incompressible data decompresses at 10000
MB/s rate, which is not far from the "memcpy". There are cases like the
[enwik9 dataset](https://mattmahoney.net/dc/textdata.html) where LZAV
provides 22% higher memory storage savings compared to LZ4.

The geomean performance of the LZAV algorithm on a variety of datasets is
550 +/- 150 MB/s compression and 3800 +/- 1300 MB/s decompression speeds,
on 4+ GHz 64-bit processors released since 2019. Note that the algorithm
exhibits adaptive qualities, and its actual performance depends on the data
being compressed. LZAV may show an exceptional performance on your specific
data, including, but not limited to: sparse databases, log files, HTML/XML
files.

It is also worth noting that compression methods like LZAV and LZ4 usually
have an advantage over dictionary- and entropy-based coding in that
hash-table-based compression has a small memory and operational overhead while
the classic LZ77 decompression has no overhead at all - this is especially
relevant for smaller data.

For a more comprehensive in-memory compression algorithms benchmark you may
visit [lzbench](https://github.com/inikep/lzbench).

### Apple clang 15.0.0 arm64, macOS 15.7, Apple M1, 3.5 GHz

Silesia compression corpus

| Compressor      | Compression | Decompression | Ratio % |
|-----------------|-------------|---------------|---------|
| **LZAV 5.8**    | 625 MB/s    | 3790 MB/s     | 39.94   |
| LZ4 1.9.4       | 700 MB/s    | 4570 MB/s     | 47.60   |
| Snappy 1.1.10   | 495 MB/s    | 3230 MB/s     | 48.22   |
| LZF 3.6         | 395 MB/s    | 800 MB/s      | 48.15   |
| **LZAV 5.8 HI** | 134 MB/s    | 3700 MB/s     | 35.12   |
| LZ4HC 1.9.4 -9  | 40 MB/s     | 4360 MB/s     | 36.75   |

### LLVM clang 19.1.7 x86-64, AlmaLinux 9.3, Xeon E-2386G (RocketLake), 5.1 GHz

Silesia compression corpus

| Compressor      | Compression | Decompression | Ratio % |
|-----------------|-------------|---------------|---------|
| **LZAV 5.8**    | 620 MB/s    | 3490 MB/s     | 39.94   |
| LZ4 1.9.4       | 848 MB/s    | 4980 MB/s     | 47.60   |
| Snappy 1.1.10   | 690 MB/s    | 3360 MB/s     | 48.22   |
| LZF 3.6         | 455 MB/s    | 1000 MB/s     | 48.15   |
| **LZAV 5.8 HI** | 115 MB/s    | 3330 MB/s     | 35.12   |
| LZ4HC 1.9.4 -9  | 43 MB/s     | 4920 MB/s     | 36.75   |

### LLVM clang-cl 18.1.8 x86-64, Windows 10, Ryzen 3700X (Zen2), 4.2 GHz

Silesia compression corpus

| Compressor      | Compression | Decompression | Ratio % |
|-----------------|-------------|---------------|---------|
| **LZAV 5.8**    | 525 MB/s    | 3060 MB/s     | 39.94   |
| LZ4 1.9.4       | 675 MB/s    | 4560 MB/s     | 47.60   |
| Snappy 1.1.10   | 415 MB/s    | 2440 MB/s     | 48.22   |
| LZF 3.6         | 310 MB/s    | 700 MB/s      | 48.15   |
| **LZAV 5.8 HI** | 116 MB/s    | 2970 MB/s     | 35.12   |
| LZ4HC 1.9.4 -9  | 36 MB/s     | 4430 MB/s     | 36.75   |

P.S. Popular Zstd's benchmark was not included here, because it is not a pure
LZ77, much harder to integrate, and has a much larger code size - a different
league, close to zlib. Here are author's Zstd measurements with
[TurboBench](https://github.com/powturbo/TurboBench/releases), on Ryzen 3700X,
on Silesia dataset:

| Compressor      | Compression | Decompression | Ratio % |
|-----------------|-------------|---------------|---------|
| zstd 1.5.5 -1   | 460 MB/s    | 1870 MB/s     | 41.0    |
| zstd 1.5.5 1    | 436 MB/s    | 1400 MB/s     | 34.6    |

## Datasets Benchmark

This section presents compression ratio comparisons for various popular
datasets. Note that each file within these datasets was compressed
individually, which contributed to the overall ratio.

| Dataset               | Size, MiB | LZAV 5.8 | LZ4 1.9.4 | Snappy 1.1.10 | LZF 3.6 | Source |
|-----------------------|-----------|----------|-----------|---------------|---------|--------|
| 4SICS 151020 PCAP     | 24.5      | 20.47    | 21.82     | 24.95         | 25.34   | [www.netresec.com](https://www.netresec.com/?page=PCAP4SICS)|
| 4SICS 151022 PCAP     | 200.0     | 36.45    | 37.35     | 40.24         | 41.37   | [www.netresec.com](https://www.netresec.com/?page=PCAP4SICS)|
| Calgary Large         | 3.1       | 44.29    | 51.97     | 51.76         | 49.07   | [data-compression.info](https://www.data-compression.info/Corpora/CalgaryCorpus/) |
| Canterbury            | 2.68      | 38.07    | 43.73     | 45.42         | 42.49   | [corpus.canterbury.ac.nz](https://corpus.canterbury.ac.nz/) |
| Canterbury Large      | 10.6      | 38.25    | 51.97     | 48.37         | 54.28   | [corpus.canterbury.ac.nz](https://corpus.canterbury.ac.nz/) |
| Canterbury Artificial | 0.29      | 33.36    | 33.74     | 36.48         | 34.66   | [corpus.canterbury.ac.nz](https://corpus.canterbury.ac.nz/) |
| employees_10KB.json   | 0.01      | 22.55    | 24.68     | 23.92         | 23.52   | [sample.json-format.com](https://sample.json-format.com/) |
| employees_100KB.json  | 0.10      | 15.96    | 17.71     | 19.02         | 21.88   | [sample.json-format.com](https://sample.json-format.com/) |
| employees_50MB.json   | 51.5      | 10.78    | 16.42     | 18.57         | 21.44   | [sample.json-format.com](https://sample.json-format.com/) |
| enwik8                | 95.4      | 44.61    | 57.26     | 56.56         | 53.95   | [www.mattmahoney.net](https://www.mattmahoney.net/dc/textdata.html) |
| enwik9                | 954.7     | 39.39    | 50.92     | 50.79         | 49.30   | [www.mattmahoney.net](https://www.mattmahoney.net/dc/textdata.html) |
| Manzini               | 855.3     | 26.98    | 37.30     | 38.57         | 39.04   | [people.unipmn.it/manzini](https://people.unipmn.it/manzini/boosting/index.html) |
| chr22.dna (Manzini)   | 33.0      | 38.79    | 52.82     | 44.53         | 55.86   | [people.unipmn.it/manzini](https://people.unipmn.it/manzini/boosting/index.html) |
| w3c2 HTML (Manzini)   | 99.4      | 11.43    | 22.20     | 25.35         | 27.20   | [people.unipmn.it/manzini](https://people.unipmn.it/manzini/boosting/index.html) |
| Silesia               | 202.1     | 39.94    | 47.60     | 48.17         | 48.15   | [github.com/MiloszKrajewski](https://github.com/MiloszKrajewski/SilesiaCorpus) |

## Notes

1. LZAV API is not equivalent to LZ4 or Snappy API. For example, the `dstlen`
parameter in the decompressor should specify the original uncompressed length,
which should have been previously stored in some way, independent of LZAV.

2. From a technical point of view, peak decompression speeds of LZAV have an
implicit limitation arising from its more complex stream format, compared to
LZ4: LZAV decompression requires more code branching. Another limiting factor
is a rather large dynamic 2-512 MiB LZ77 window which is not CPU
cache-friendly. On the other hand, without these features it would not be
possible to achieve competitive compression ratios while having fast
compression speeds.

3. LZAV supports compression of continuous data blocks of up to 2 GB. Larger
data should be compressed in chunks of at least 16 MB. Using smaller chunks
may reduce the achieved compression ratio.

## Thanks

* [Paul Dreik](https://github.com/pauldreik), for finding memcpy UB in the
decompressor.
