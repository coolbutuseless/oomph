
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

| expression         |   min |   median |      itr/sec | mem_alloc |
|:-------------------|------:|---------:|-------------:|----------:|
| match(t0, nms)     | 876µs |   1.16ms |     860.3279 |    3.82MB |
| mph_match(t0, mph) | 205ns | 286.85ns | 2885114.5335 |    3.97KB |

``` r
bench::mark(
  match(t1, nms),
  mph_match(t1, mph)
)[, 1:5] |> knitr::kable()
```

| expression         |      min |   median |     itr/sec | mem_alloc |
|:-------------------|---------:|---------:|------------:|----------:|
| match(t1, nms)     |   5.46ms |   6.36ms |     157.299 |    7.82MB |
| mph_match(t1, mph) | 327.83ns | 450.76ns | 2199657.545 |        0B |

``` r
bench::mark(
  match(t2, nms),
  mph_match(t2, mph)
)[, 1:5] |> knitr::kable()
```

| expression         |    min | median |     itr/sec | mem_alloc |
|:-------------------|-------:|-------:|------------:|----------:|
| match(t2, nms)     | 5.44ms | 6.25ms |    160.3913 |    7.82MB |
| mph_match(t2, mph) | 2.21µs | 2.38µs | 395176.2420 |      448B |

``` r
bench::mark(
  match(t3, nms),
  mph_match(t3, mph)
)[, 1:5] |> knitr::kable()
```

| expression         |     min |  median |    itr/sec | mem_alloc |
|:-------------------|--------:|--------:|-----------:|----------:|
| match(t3, nms)     |  5.72ms |  6.19ms |   159.9975 |    7.83MB |
| mph_match(t3, mph) | 32.19µs | 32.72µs | 30132.1977 |    3.95KB |

## Vector subsetting - Extract 100 elements of a `vector` by name

``` r
bench::mark(
  big_vector[t2],
  big_vector[mph_match(t2, mph)]
)[, 1:5] |> knitr::kable()
```

| expression                       |    min | median |     itr/sec | mem_alloc |
|:---------------------------------|-------:|-------:|------------:|----------:|
| big_vector\[t2\]                 | 5.69ms | 6.16ms |    161.8019 |    7.82MB |
| big_vector\[mph_match(t2, mph)\] | 3.08µs | 3.28µs | 286025.9634 |     1.7KB |

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
| Standard R            | 5.63ms |  5.87ms |    166.9113 |    7.82MB |
| R hashed environment  | 18.2µs | 18.53µs |  52214.6320 |      848B |
| \[\] and mph indexing | 3.44µs |  3.65µs | 249600.0729 |    2.09KB |

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

| expression        |   min |   median |    itr/sec | mem_alloc |
|:------------------|------:|---------:|-----------:|----------:|
| mph_init(nms1k)   | 194µs | 206.11µs | 4747.72648 |      12KB |
| mph_init(nms10k)  |   2ms |   2.08ms |   69.45488 |  167.16KB |
| mph_init(nms100k) |  23ms |  26.85ms |   37.39611 |    1.38MB |

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

| expression     |     min | median |    itr/sec | mem_alloc |
|:---------------|--------:|-------:|-----------:|----------:|
| baseR          | 138.7µs |  162µs |   369.0988 |   558.6KB |
| oomph          |  30.9µs | 31.8µs | 30296.5325 |    19.7KB |
| oomph + insitu |  24.1µs | 24.7µs | 39431.5282 |    14.6KB |

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
| baseR          | 166.08ms | 209.66ms |   5.121019 |   134.2MB |
| oomph          |   1.45ms |   1.51ms | 478.109260 |    20.2KB |
| oomph + insitu |   1.41ms |   1.49ms | 539.650100 |    11.3KB |
