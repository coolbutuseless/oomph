
<!-- README.md is generated from README.Rmd. Please edit that file -->

# oomph

<!-- badges: start -->

![](https://img.shields.io/badge/cool-useless-green.svg)
[![R-CMD-check](https://github.com/coolbutuseless/oomph/actions/workflows/R-CMD-check.yaml/badge.svg)](https://github.com/coolbutuseless/oomph/actions/workflows/R-CMD-check.yaml)
<!-- badges: end -->

`oomph` is a package for fast string matching within a static set of
strings. This is useful for fast named look-up in fixed lists and
vectors.

Internally this uses a hash-map in C to map strings to integers. In R,
this appears as a minimal perfect hash where each string maps to its
index, and unknown strings return `NA`

The hashed look-up can be more than **1000x** faster than R’s standard
look-up method (depending on number of elements in original object and
the number of elements to extract).

## What’s in the box

- `mph <- mph_init(s, size_factor)` initialise a hash with the given set
  of strings
  - Using a larger `size_factor` (than the default of `1`) decreases the
    number of hash collisions, and can make other operations faster at
    the cost or more memory being allocated.
- `mph_match(s, mph)` find the indices of the strings `s` (equivalent to
  R’s `match()`)

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
N <- 500000
set.seed(1)

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# 500k random names
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
nms <- vapply(seq(N), \(i) paste(sample(c(letters, LETTERS, 0:9), 10, T), collapse = ""), character(1))
head(nms)
```

    #> [1] "4dMaH8wQnr" "6YGuuP1Tjg" "iouKOyTKKH" "P9yRoGtIfj" "PLUtBt5R4w"
    #> [6] "f4NRy23f5M"

``` r
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# A big named vector and named list (each with 500k elements)
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
big_vector <- seq(N)
big_list   <- as.list(seq(N))

names(big_vector) <- nms
names(big_list  ) <- nms

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Probe sets to use for testing
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
t0  <- sample(nms,    1, replace = TRUE)
t1  <- sample(nms,   10, replace = TRUE)
t2  <- sample(nms,  100, replace = TRUE)
t3  <- sample(nms, 1000, replace = TRUE)

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# By default, the number of hash buckets is the same as the number of 
# strings.  To reduce the possibility of hash collisions (and possibly make look-ups
# faster), the number of hash buckets can be changed using the 'size_factor'
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mph <- mph_init(nms) # Allocate exactly length(nms) buckets
```

## Compare `match()` with `mph_match()`

``` r
bench::mark(
  match(t0, nms),
  mph_match(t0, mph)
)[, 1:5] |> knitr::kable()
```

| expression         |      min |   median |      itr/sec | mem_alloc |
|:-------------------|---------:|---------:|-------------:|----------:|
| match(t0, nms)     |   1.04ms |   1.13ms |     869.1267 |    3.82MB |
| mph_match(t0, mph) | 205.07ns | 287.02ns | 3092803.9776 |    3.97KB |

``` r
bench::mark(
  match(t1, nms),
  mph_match(t1, mph)
)[, 1:5] |> knitr::kable()
```

| expression         |    min |   median |      itr/sec | mem_alloc |
|:-------------------|-------:|---------:|-------------:|----------:|
| match(t1, nms)     | 5.42ms |   5.77ms |     172.2174 |    7.82MB |
| mph_match(t1, mph) |  328ns | 410.01ns | 2208594.9456 |        0B |

``` r
bench::mark(
  match(t2, nms),
  mph_match(t2, mph)
)[, 1:5] |> knitr::kable()
```

| expression         |    min | median |     itr/sec | mem_alloc |
|:-------------------|-------:|-------:|------------:|----------:|
| match(t2, nms)     | 5.23ms | 5.74ms |    172.2227 |    7.82MB |
| mph_match(t2, mph) | 2.05µs | 2.17µs | 429477.2517 |      448B |

``` r
bench::mark(
  match(t3, nms),
  mph_match(t3, mph)
)[, 1:5] |> knitr::kable()
```

| expression         |     min |  median |    itr/sec | mem_alloc |
|:-------------------|--------:|--------:|-----------:|----------:|
| match(t3, nms)     |  5.23ms |  5.46ms |   183.0433 |    7.83MB |
| mph_match(t3, mph) | 28.99µs | 29.52µs | 33044.5556 |    3.95KB |

## Vector subsetting - Extract 100 elements of a `vector` by name

``` r
bench::mark(
  big_vector[t2],
  big_vector[mph_match(t2, mph)]
)[, 1:5] |> knitr::kable()
```

| expression                       |    min | median |     itr/sec | mem_alloc |
|:---------------------------------|-------:|-------:|------------:|----------:|
| big_vector\[t2\]                 | 5.19ms | 5.46ms |    182.1493 |    7.82MB |
| big_vector\[mph_match(t2, mph)\] | 2.87µs | 3.03µs | 308181.2405 |     1.7KB |

## List subsetting - Extract 100 elements of a `list` by name

Also compare to using hashed named lookup in a standard R environment

``` r
ee <- as.environment(big_list)

bench::mark(
  `Standard R`           = big_list[t2],
  `R hashed environment` = mget(t2, ee),
  `[] and mph indexing`  = big_list[mph_match(t2, mph)]
)[, 1:5] |> knitr::kable()
```

| expression            |    min |  median |     itr/sec | mem_alloc |
|:----------------------|-------:|--------:|------------:|----------:|
| Standard R            | 5.37ms |  5.42ms |    182.4047 |    7.82MB |
| R hashed environment  | 17.1µs | 17.43µs |  55575.2344 |      848B |
| \[\] and mph indexing |  3.2µs |  3.36µs | 272395.5087 |    2.09KB |

## Time taken to build the hash

``` r
set.seed(1)
chrs <- c(letters, LETTERS, 0:9)
N <- 1000
nms1k <- vapply(seq(N), \(i) paste(sample(chrs, 10, T), collapse = ""), character(1))

N <- 10000
nms10k <- vapply(seq(N), \(i) paste(sample(chrs, 10, T), collapse = ""), character(1))

N <- 100000
nms100k <- vapply(seq(N), \(i) paste(sample(chrs, 10, T), collapse = ""), character(1))

bench::mark(
  mph_init(nms1k),
  mph_init(nms10k),
  mph_init(nms100k),
  check = FALSE
)[, 1:5] |> knitr::kable()
```

| expression        |      min |   median |    itr/sec | mem_alloc |
|:------------------|---------:|---------:|-----------:|----------:|
| mph_init(nms1k)   | 176.01µs | 182.41µs | 5335.00378 |      12KB |
| mph_init(nms10k)  |   1.76ms |   1.84ms |  149.55035 |  167.16KB |
| mph_init(nms100k) |     19ms |  20.92ms |   46.98398 |    1.38MB |

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
  baseR            = {i <- match(random_nms, nms); counts[i] <- counts[i] + 1},
  oomph            = {i <- mph_match(random_nms, mph); counts[i] <- counts[i] + 1},
  `oomph + insitu` = {br_add(counts, 1, idx =  mph_match(random_nms, mph))},
  check = FALSE
)[, 1:5] |> knitr::kable()
```

| expression     |    min |  median |  itr/sec | mem_alloc |
|:---------------|-------:|--------:|---------:|----------:|
| baseR          |  139µs | 165.9µs |  5196.78 |   558.6KB |
| oomph          | 30.4µs |    32µs | 30054.18 |    19.7KB |
| oomph + insitu | 23.5µs |  24.2µs | 39493.92 |    14.5KB |

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

| expression     |      min |   median |    itr/sec | mem_alloc |
|:---------------|---------:|---------:|-----------:|----------:|
| baseR          | 166.63ms | 212.91ms |   5.049908 |   134.2MB |
| oomph          |   1.48ms |   1.66ms | 142.149867 |    20.2KB |
| oomph + insitu |   1.39ms |   1.42ms | 533.828883 |    11.3KB |
