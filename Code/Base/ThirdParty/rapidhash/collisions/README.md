
__collisionsTest__ is a brute force hash analyzer originally published within [xxHash](https://github.com/Cyan4973/xxHash/tree/dev/tests/collisions).
It will measure a 64-bit hash algorithm's collision rate by generating billions of hashes and counting collisions.

#### Usage

```
usage: ./collisionsTest [hashName] [opt]

list of hashNames: (...)

Optional parameters:
  --nbh=NB       Select nb of hashes to generate (25769803776 by default)
  --filter       Enable the filter. Slower, but reduces memory usage for same nb of hashes.
  --threadlog=NB Use 2^NB threads
  --len=NB       Select length of input (255 bytes by default)
```

#### Measurements

Here are some results for rapidhash, using $0$ as seed:

| Input Len | Nb Hashes | Expected | Nb Collisions |
| ---  | ---   | ---   | --- |
|    5 | 15 Gi |   7.0 |   6 |
|    6 | 15 Gi |   7.0 |   6 |
|    7 | 15 Gi |   7.0 |   5 |
|    8 | 15 Gi |   7.0 |  13 |
|    9 | 15 Gi |   7.0 |  10 |
|   10 | 15 Gi |   7.0 |   6 |
|   11 | 15 Gi |   7.0 |   6 |
|   12 | 15 Gi |   7.0 |   3 |
|   13 | 15 Gi |   7.0 |   5 |
|   14 | 15 Gi |   7.0 |  13 |
|   15 | 15 Gi |   7.0 |   6 |
|   16 | 15 Gi |   7.0 |   7 |
|   17 | 15 Gi |   7.0 |   6 |
|   18 | 15 Gi |   7.0 |   9 |
|   19 | 15 Gi |   7.0 |  10 |
|   20 | 15 Gi |   7.0 |   3 |
|   21 | 15 Gi |   7.0 |   9 |
|   22 | 15 Gi |   7.0 |  11 |
|   23 | 15 Gi |   7.0 |   3 |
|   24 | 15 Gi |   7.0 |   7 |
|   25 | 15 Gi |   7.0 |   4 |
|   26 | 15 Gi |   7.0 |   4 |
|   27 | 15 Gi |   7.0 |  14 |
|   28 | 15 Gi |   7.0 |   6 |
|   29 | 15 Gi |   7.0 |   4 |
|   30 | 15 Gi |   7.0 |  10 |
|   31 | 15 Gi |   7.0 |   4 |
|   32 | 15 Gi |   7.0 |  10 |
|   33 | 15 Gi |   7.0 |   8 |
|   34 | 15 Gi |   7.0 |   5 |
|   35 | 15 Gi |   7.0 |   4 |
|   36 | 15 Gi |   7.0 |  10 |
|   37 | 15 Gi |   7.0 |  13 |
|   38 | 15 Gi |   7.0 |   7 |
|   39 | 15 Gi |   7.0 |   6 |
|   40 | 15 Gi |   7.0 |   4 |
|   41 | 15 Gi |   7.0 |   5 |
|   42 | 15 Gi |   7.0 |   7 |
|   43 | 15 Gi |   7.0 |   8 |
|   44 | 15 Gi |   7.0 |   6 |
|   45 | 15 Gi |   7.0 |   4 |
|   46 | 15 Gi |   7.0 |   7 |
|   47 | 15 Gi |   7.0 |  10 |
|   48 | 15 Gi |   7.0 |   7 |
|   49 | 15 Gi |   7.0 |   6 |
|   50 | 15 Gi |   7.0 |   3 |
|   51 | 15 Gi |   7.0 |  10 |
|   52 | 15 Gi |   7.0 |   6 |
|   53 | 15 Gi |   7.0 |   6 |
|   54 | 15 Gi |   7.0 |   9 |
|   55 | 15 Gi |   7.0 |   9 |
|   56 | 15 Gi |   7.0 |  13 |
|   57 | 15 Gi |   7.0 |  11 |
|   58 | 15 Gi |   7.0 |   8 |
|   59 | 15 Gi |   7.0 |  12 |
|   60 | 15 Gi |   7.0 |  14 |
|   61 | 15 Gi |   7.0 |   7 |
|   62 | 15 Gi |   7.0 |   8 |
|   63 | 15 Gi |   7.0 |   9 |
|   64 | 15 Gi |   7.0 |   6 |
|   72 | 15 Gi |   7.0 |   7 |
|   80 | 15 Gi |   7.0 |  11 |
|   88 | 15 Gi |   7.0 |   7 |
|   96 | 15 Gi |   7.0 |   6 |
|  104 | 15 Gi |   7.0 |   8 |
|  112 | 15 Gi |   7.0 |   6 |
|  120 | 15 Gi |   7.0 |   8 |
|  128 | 15 Gi |   7.0 |   6 |
|    8 | 62 Gi | 120.1 | 131 |
|   12 | 62 Gi | 120.1 | 122 |
|   16 | 62 Gi | 120.1 |  97 |
|   20 | 62 Gi | 120.1 | 121 |
|   24 | 62 Gi | 120.1 | 125 |
|   32 | 62 Gi | 120.1 | 131 |
|   40 | 62 Gi | 120.1 | 117 |
|   48 | 62 Gi | 120.1 | 146 |
|   56 | 62 Gi | 120.1 | 138 |
|   64 | 62 Gi | 120.1 | 162 |
|   72 | 62 Gi | 120.1 | 177 |
|   80 | 62 Gi | 120.1 | 165 |
|   96 | 62 Gi | 120.1 | 180 |
|  112 | 62 Gi | 120.1 | 206 |
|  120 | 62 Gi | 120.1 | 168 |
|  128 | 62 Gi | 120.1 | 201 |
