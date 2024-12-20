
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
| match(t1, nms)      | 250µs |  330µs |    3009.918 |    1.53MB |
| mph_match(t1, mph1) | 246ns |  328ns | 2578158.441 |    3.97KB |
| mph_match(t1, mph4) | 246ns |  328ns | 2567845.396 |        0B |

``` r
bench::mark(
  match(t2, nms),
  mph_match(t2, mph1),
  mph_match(t2, mph4)
)[, 1:5] |> knitr::kable()
```

| expression          |   min |   median |      itr/sec | mem_alloc |
|:--------------------|------:|---------:|-------------:|----------:|
| match(t2, nms)      | 1.5ms |   1.65ms |     602.3474 |    3.53MB |
| mph_match(t2, mph1) | 369ns | 450.99ns | 2067228.4145 |        0B |
| mph_match(t2, mph4) | 369ns | 450.99ns | 2100029.3376 |        0B |

``` r
bench::mark(
  match(t3, nms),
  mph_match(t3, mph1),
  mph_match(t3, mph4)
)[, 1:5] |> knitr::kable()
```

| expression          |    min | median |     itr/sec | mem_alloc |
|:--------------------|-------:|-------:|------------:|----------:|
| match(t3, nms)      | 1.49ms | 1.64ms |    610.9512 |    3.53MB |
| mph_match(t3, mph1) | 2.34µs |  2.5µs | 379768.1698 |      448B |
| mph_match(t3, mph4) | 2.01µs | 2.17µs | 437718.4020 |      448B |

``` r
bench::mark(
  match(t4, nms),
  mph_match(t4, mph1),
  mph_match(t4, mph4)
)[, 1:5] |> knitr::kable()
```

| expression          |     min |  median |    itr/sec | mem_alloc |
|:--------------------|--------:|--------:|-----------:|----------:|
| match(t4, nms)      |  1.48ms |  1.65ms |   598.8589 |    3.54MB |
| mph_match(t4, mph1) | 32.27µs | 32.92µs | 29440.8877 |    3.95KB |
| mph_match(t4, mph4) | 22.88µs | 23.29µs | 41488.8817 |    3.95KB |

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
| big_vector\[t3\]                  |  1.6ms | 1.92ms |    525.7467 |    3.53MB |
| big_vector\[mph_match(t3, mph1)\] | 3.08µs | 3.53µs | 262274.1478 |     1.7KB |
| big_vector\[mph_match(t3, mph4)\] | 2.83µs | 3.16µs | 291940.4233 |     1.7KB |
| mph_subset(t3, big_vector, mph1)  | 2.62µs | 2.87µs | 333000.1740 |  785.86KB |
| mph_subset(t3, big_vector, mph4)  |  2.3µs | 2.46µs | 388045.2228 |      448B |

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

| expression            |    min | median |     itr/sec | mem_alloc |
|:----------------------|-------:|-------:|------------:|----------:|
| Standard R            | 1.49ms | 1.67ms |    588.3338 |    3.53MB |
| \[\] and mph indexing | 3.53µs | 3.77µs | 250856.0742 |    2.09KB |
| \[\] and mph indexing | 3.16µs |  3.4µs | 276699.4996 |    2.09KB |
| custom mph method     | 3.81µs | 3.98µs | 241930.4152 |      848B |
| custom mph method     | 3.12µs | 3.32µs | 286566.9910 |      848B |

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
| Standard R            |   1.5ms |  1.67ms |    596.4754 |    3.53MB |
| \[\] and mph indexing |  3.57µs |  3.77µs | 251919.9620 |    2.09KB |
| \[\] and mph indexing |   3.2µs |   3.4µs | 277421.8646 |    2.09KB |
| custom mph method     |  3.77µs |  4.02µs | 240677.9688 |      848B |
| custom mph method     |  3.12µs |  3.32µs | 286297.5708 |      848B |
| R hashed environment  | 13.98µs | 14.27µs |  67977.3977 |      848B |

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
| mph_init(nms1k)   | 185.07µs | 193.19µs | 5102.26872 |      12KB |
| mph_init(nms10k)  |   1.84ms |   1.92ms |  243.40399 |  167.16KB |
| mph_init(nms100k) |  20.37ms |  22.18ms |   44.44291 |    1.38MB |

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
  baseR = {i <- match(random_nms, nms); counts[i] <- counts[i] + 1},
  oomph = {i <- mph_match(random_nms, mph); counts[i] <- counts[i] + 1},
  `oomph + insitu` = {br_add(counts, 1, idx =  mph_match(random_nms, mph))},
  check = FALSE
)[, 1:5] |> knitr::kable()
```

| expression     |     min |  median |   itr/sec | mem_alloc |
|:---------------|--------:|--------:|----------:|----------:|
| baseR          | 149.5µs | 176.3µs |  5326.015 |   558.6KB |
| oomph          |  40.8µs |  42.4µs | 22879.782 |    19.7KB |
| oomph + insitu |  34.3µs |    35µs | 27779.035 |    14.5KB |

``` r
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Updating within a for loop
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bench::mark(
  baseR = {
    for (nm in random_nms) {
      i <- match(nm, nms)
      counts[i] <- counts[i] + 1
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

| expression     |     min |  median |   itr/sec | mem_alloc |
|:---------------|--------:|--------:|----------:|----------:|
| baseR          | 64.16ms | 67.48ms |  12.01367 |   134.2MB |
| oomph          |  1.42ms |   1.5ms | 552.59511 |    20.2KB |
| oomph + insitu |   1.4ms |  1.49ms | 579.10510 |    11.3KB |
