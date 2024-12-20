
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
| match(t1, nms)      | 250µs |  345µs |    2896.097 |    1.53MB |
| mph_match(t1, mph1) | 205ns |  287ns | 2692843.497 |    3.97KB |
| mph_match(t1, mph4) | 205ns |  287ns | 2824600.661 |        0B |

``` r
bench::mark(
  match(t2, nms),
  mph_match(t2, mph1),
  mph_match(t2, mph4)
)[, 1:5] |> knitr::kable()
```

| expression          |      min |   median |      itr/sec | mem_alloc |
|:--------------------|---------:|---------:|-------------:|----------:|
| match(t2, nms)      |   1.65ms |   1.78ms |     552.5642 |    3.53MB |
| mph_match(t2, mph1) | 287.02ns | 410.01ns | 2291186.5842 |        0B |
| mph_match(t2, mph4) |    328ns | 410.01ns | 2243043.5560 |        0B |

``` r
bench::mark(
  match(t3, nms),
  mph_match(t3, mph1),
  mph_match(t3, mph4)
)[, 1:5] |> knitr::kable()
```

| expression          |    min | median |     itr/sec | mem_alloc |
|:--------------------|-------:|-------:|------------:|----------:|
| match(t3, nms)      | 1.63ms | 1.78ms |    555.8609 |    3.53MB |
| mph_match(t3, mph1) |  2.3µs |  2.5µs | 384841.7947 |      448B |
| mph_match(t3, mph4) | 2.05µs | 2.21µs | 428400.8371 |      448B |

``` r
bench::mark(
  match(t4, nms),
  mph_match(t4, mph1),
  mph_match(t4, mph4)
)[, 1:5] |> knitr::kable()
```

| expression          |     min |  median |    itr/sec | mem_alloc |
|:--------------------|--------:|--------:|-----------:|----------:|
| match(t4, nms)      |  1.57ms |  1.77ms |   554.0595 |    3.54MB |
| mph_match(t4, mph1) | 32.14µs | 32.76µs | 29829.8016 |    3.95KB |
| mph_match(t4, mph4) | 22.67µs | 23.12µs | 42251.9990 |    3.95KB |

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
| big_vector\[t3\]                  |  1.6ms | 1.81ms |    544.8513 |    3.53MB |
| big_vector\[mph_match(t3, mph1)\] | 3.03µs | 3.28µs | 285625.0546 |     1.7KB |
| big_vector\[mph_match(t3, mph4)\] | 2.79µs | 2.99µs | 308091.2447 |     1.7KB |
| mph_subset(t3, big_vector, mph1)  | 2.75µs | 2.95µs | 325728.6315 |  785.86KB |
| mph_subset(t3, big_vector, mph4)  | 2.34µs | 2.54µs | 375155.3152 |      448B |

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
| Standard R            | 1.62ms | 1.77ms |    551.8103 |    3.53MB |
| \[\] and mph indexing | 3.44µs | 3.69µs | 253797.6698 |    2.09KB |
| \[\] and mph indexing |  3.2µs | 3.44µs | 276022.7234 |    2.09KB |
| custom mph method     | 3.81µs | 4.02µs | 237062.1764 |      848B |
| custom mph method     |  3.2µs |  3.4µs | 277313.0326 |      848B |

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

| expression            |    min |  median |     itr/sec | mem_alloc |
|:----------------------|-------:|--------:|------------:|----------:|
| Standard R            | 1.55ms |  1.79ms |    547.6771 |    3.53MB |
| \[\] and mph indexing | 3.44µs |  3.73µs | 254812.7429 |    2.09KB |
| \[\] and mph indexing |  3.2µs |   3.4µs | 277596.4925 |    2.09KB |
| custom mph method     | 3.81µs |  4.02µs | 238649.4639 |      848B |
| custom mph method     |  3.2µs |   3.4µs | 273530.3028 |      848B |
| R hashed environment  | 14.1µs | 14.43µs |  67096.3519 |      848B |

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
| mph_init(nms1k)   | 174.74µs | 188.44µs | 5186.36568 |      12KB |
| mph_init(nms10k)  |   1.74ms |   1.82ms |  199.28013 |  167.16KB |
| mph_init(nms100k) |  19.37ms |  21.21ms |   46.29888 |    1.38MB |
