
<!-- README.md is generated from README.Rmd. Please edit that file -->

# oomph

<!-- badges: start -->

![](https://img.shields.io/badge/cool-useless-green.svg)
[![R-CMD-check](https://github.com/coolbutuseless/oomph/actions/workflows/R-CMD-check.yaml/badge.svg)](https://github.com/coolbutuseless/oomph/actions/workflows/R-CMD-check.yaml)
<!-- badges: end -->

`oomph` is a package for:

- fast string matching
- fast named look-up of data in static lists and vectors.

Internally this uses a hash-map in C to map strings to integers. In R,
this appears as a minimal perfect hash where each string maps to its
index, and unknown strings return `NA`

The hashed look-up can be **1000x** faster than R’s standard look-up
method (depending on number of elements in original object and the
number of elements to extract).

## What’s in the box

- `mph <- mph_init(s, size_factor)` initialise a hash with the given set
  of strings
  - Using a larger `size_factor` (than the default of `1`) decreases the
    number of hash collisions, and can make other operations faster at
    the cost or more memory being allocated.
- `mph_match(s, mph)` find the indices of the strings `s` (equivalent to
  R’s `match()`)
- `mph_subset(nms, x, mph)` extract elements with names `nms` from
  vector/list `x` using the given hash object `mph`.

## Installation

You can install from [GitHub](https://github.com/coolbutuseless/oomph)
with:

``` r
# install.package('remotes')
remotes::install_github('coolbutuseless/oomph')
```

## Setup test data

``` r
library(oomph)
N <- 200000
set.seed(1)

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# 200,000 random names
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
nms <- vapply(seq(N), \(i) paste(sample(c(letters, LETTERS, 0:9), 10, T), collapse = ""), character(1))
head(nms)
```

    #> [1] "4dMaH8wQnr" "6YGuuP1Tjg" "iouKOyTKKH" "P9yRoGtIfj" "PLUtBt5R4w"
    #> [6] "f4NRy23f5M"

``` r
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# A big vector and a big list (each with 200,000 elements)
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
big_vector <- seq(N)
big_list   <- as.list(seq(N))

names(big_vector) <- nms
names(big_list  ) <- nms

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Probe sets to use for testing
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
t1  <- sample(nms,    1, replace = TRUE)
t2  <- sample(nms,   10, replace = TRUE)
t3  <- sample(nms,  100, replace = TRUE)
t4  <- sample(nms, 1000, replace = TRUE)

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# By default, the number of hash buckets is the same as the number of 
# strings.  To reduce the possibility of hash collisions (and possibly make look-ups
# faster), the number of hash buckets can be changed using the 'size_factor'
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mph1 <- mph_init(nms)                  # Allocate exactly length(nms) buckets
mph4 <- mph_init(nms, size_factor = 4) # Allocate: 4 * length(nms) buckets
```

## Compare `match()` with `mph_match()`

``` r
bench::mark(
  match(t1, nms),
  mph_match(t1, mph1),
  mph_match(t1, mph4)
)[, 1:5] |> knitr::kable()
```

| expression          |   min | median |     itr/sec | mem_alloc |
|:--------------------|------:|-------:|------------:|----------:|
| match(t1, nms)      | 250µs |  350µs |    2799.844 |    1.53MB |
| mph_match(t1, mph1) | 205ns |  287ns | 2887741.050 |    3.97KB |
| mph_match(t1, mph4) | 205ns |  287ns | 2701763.840 |        0B |

``` r
bench::mark(
  match(t2, nms),
  mph_match(t2, mph1),
  mph_match(t2, mph4)
)[, 1:5] |> knitr::kable()
```

| expression          |      min | median |      itr/sec | mem_alloc |
|:--------------------|---------:|-------:|-------------:|----------:|
| match(t2, nms)      |   1.57ms |  1.8ms |     541.6342 |    3.53MB |
| mph_match(t2, mph1) |    328ns |  451ns | 2152118.1268 |        0B |
| mph_match(t2, mph4) | 328.06ns |  410ns | 2181609.3357 |        0B |

``` r
bench::mark(
  match(t3, nms),
  mph_match(t3, mph1),
  mph_match(t3, mph4)
)[, 1:5] |> knitr::kable()
```

| expression          |    min | median |     itr/sec | mem_alloc |
|:--------------------|-------:|-------:|------------:|----------:|
| match(t3, nms)      | 1.53ms | 1.72ms |    572.7102 |    3.53MB |
| mph_match(t3, mph1) | 2.34µs |  2.5µs | 378618.0135 |      448B |
| mph_match(t3, mph4) | 2.01µs | 2.17µs | 444873.5745 |      448B |

``` r
bench::mark(
  match(t4, nms),
  mph_match(t4, mph1),
  mph_match(t4, mph4)
)[, 1:5] |> knitr::kable()
```

| expression          |     min |  median |    itr/sec | mem_alloc |
|:--------------------|--------:|--------:|-----------:|----------:|
| match(t4, nms)      |  1.54ms |  1.76ms |   549.2129 |    3.54MB |
| mph_match(t4, mph1) | 32.27µs |  32.8µs | 29532.3482 |    3.95KB |
| mph_match(t4, mph4) | 22.51µs | 23.41µs | 41376.6055 |    3.95KB |

## Vector subsetting - Extract 100 elements of a `vector` by name

``` r
bench::mark(
  big_vector[t3],
  big_vector[mph_match(t3, mph1)],
  big_vector[mph_match(t3, mph4)],
  mph_subset(t3, big_vector, mph1),
  mph_subset(t3, big_vector, mph4),
  check = F
)[, 1:5] |> knitr::kable()
```

| expression                        |    min | median |     itr/sec | mem_alloc |
|:----------------------------------|-------:|-------:|------------:|----------:|
| big_vector\[t3\]                  | 1.59ms |  1.8ms |    532.5703 |    3.53MB |
| big_vector\[mph_match(t3, mph1)\] | 3.08µs | 3.32µs | 280377.8390 |     1.7KB |
| big_vector\[mph_match(t3, mph4)\] | 2.75µs | 2.95µs | 314764.7050 |     1.7KB |
| mph_subset(t3, big_vector, mph1)  | 2.54µs | 2.71µs | 348243.9138 |  785.86KB |
| mph_subset(t3, big_vector, mph4)  | 2.25µs | 2.38µs | 394107.8343 |      448B |

## List subsetting - Extract 100 elements of a `list` by name

``` r
bench::mark(
  `Standard R`           = big_list[t3],
  `[] and mph indexing`  = big_list[mph_match(t3, mph1)],
  `[] and mph indexing`  = big_list[mph_match(t3, mph4)],
  `custom mph method`    = mph_subset(t3, big_list, mph1),
  `custom mph method`    = mph_subset(t3, big_list, mph4),
  check = FALSE
)[, 1:5] |> knitr::kable()
```

| expression            |    min | median |    itr/sec | mem_alloc |
|:----------------------|-------:|-------:|-----------:|----------:|
| Standard R            | 1.59ms | 1.72ms |    571.662 |    3.53MB |
| \[\] and mph indexing | 3.48µs | 3.73µs | 255325.233 |    2.09KB |
| \[\] and mph indexing | 3.16µs | 3.36µs | 280625.266 |    2.09KB |
| custom mph method     | 3.77µs | 3.98µs | 239653.754 |      848B |
| custom mph method     | 3.12µs | 3.32µs | 283301.001 |      848B |

## Comparison of hashed list subsetting to R’s hashed look-ups in environments

``` r
ee <- as.environment(big_list)

bench::mark(
  `Standard R`           = big_list[t3],
  `[] and mph indexing`  = big_list[mph_match(t3, mph1)],
  `[] and mph indexing`  = big_list[mph_match(t3, mph4)],
  `custom mph method`    = mph_subset(t3, big_list, mph1),
  `custom mph method`    = mph_subset(t3, big_list, mph4),
  `R hashed environment` = mget(t3, ee),
  check = FALSE
)[, 1:5] |> knitr::kable()
```

| expression            |     min |  median |     itr/sec | mem_alloc |
|:----------------------|--------:|--------:|------------:|----------:|
| Standard R            |  1.59ms |  1.74ms |    561.3185 |    3.53MB |
| \[\] and mph indexing |  3.48µs |  3.73µs | 257093.8887 |    2.09KB |
| \[\] and mph indexing |  3.12µs |   3.4µs | 280065.5079 |    2.09KB |
| custom mph method     |  3.73µs |  3.94µs | 239473.1177 |      848B |
| custom mph method     |  3.12µs |  3.32µs | 286683.5258 |      848B |
| R hashed environment  | 13.86µs | 14.23µs |  68116.9291 |      848B |

## Time taken to build the hash

``` r
set.seed(1)
N <- 1000
nms1k <- vapply(seq(N), \(i) paste(sample(c(letters, LETTERS, 0:9), 10, T), collapse = ""), character(1))

N <- 10000
nms10k <- vapply(seq(N), \(i) paste(sample(c(letters, LETTERS, 0:9), 10, T), collapse = ""), character(1))

N <- 100000
nms100k <- vapply(seq(N), \(i) paste(sample(c(letters, LETTERS, 0:9), 10, T), collapse = ""), character(1))

bench::mark(
  mph_init(nms1k),
  mph_init(nms10k),
  mph_init(nms100k),
  check = FALSE
)[, 1:5] |> knitr::kable()
```

| expression        |      min |   median |    itr/sec | mem_alloc |
|:------------------|---------:|---------:|-----------:|----------:|
| mph_init(nms1k)   | 187.78µs | 197.05µs | 4990.80035 |      12KB |
| mph_init(nms10k)  |   1.87ms |   1.96ms |  138.27884 |  167.16KB |
| mph_init(nms100k) |  19.82ms |  22.95ms |   42.49533 |    1.38MB |

## Billion Row Challenge indexing

The following example is a part of the [billion row
challenge](https://github.com/jrosell/1br).

In this example, we are attempting to keep a streaming tally of the
3-letter codes which are seen.

``` r
library(oomph)
library(insitu)

nms <- expand.grid(LETTERS, LETTERS, LETTERS) |> 
  apply(1, paste0, collapse = "")

counts <- numeric(length(nms))
names(counts) <- nms
mph <- mph_init(nms)

set.seed(1)
random_nms <- sample(nms, 1000)


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# updating in bulk
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bench::mark(
  baseR = {i <- match(random_nms, names(counts)); counts[i] <- counts[i] + 1},
  oomph = {i <- mph_match(random_nms, mph); counts[i] <- counts[i] + 1},
  `oomph + insitu` = {br_add(counts, 1, idx =  mph_match(random_nms, mph))},
  check = FALSE
)[, 1:5] |> knitr::kable()
```

| expression     |     min |  median |   itr/sec | mem_alloc |
|:---------------|--------:|--------:|----------:|----------:|
| baseR          | 145.8µs | 172.3µs |  5748.421 |   558.6KB |
| oomph          |  41.1µs |  42.4µs | 23019.751 |    19.7KB |
| oomph + insitu |  34.2µs |  34.9µs | 27923.299 |    14.5KB |

``` r
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Updating within a for loop
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bench::mark(
  baseR = {
    for (nm in random_nms) {
      counts[nm] <- counts[nm] + 1
    }
  },
  oomph = {
    for (nm in random_nms) {
      i <- mph_match(nm, mph)
      counts[i] <- counts[i] + 1
    }
  },
  `oomph + insitu` = {
    for (nm in random_nms) {
      br_add(counts, 1, idx = mph_match(nm, mph))
    }
  },
  check = FALSE
)[, 1:5] |> knitr::kable()
```

    #> Warning: Some expressions had a GC in every iteration; so filtering is
    #> disabled.

| expression     |      min |   median |    itr/sec | mem_alloc |
|:---------------|---------:|---------:|-----------:|----------:|
| baseR          | 284.32ms | 288.86ms |   3.461934 |   536.6MB |
| oomph          |   1.43ms |   1.51ms | 546.317735 |    20.2KB |
| oomph + insitu |   1.42ms |   1.49ms | 577.721057 |    11.3KB |
