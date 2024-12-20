
<!-- README.md is generated from README.Rmd. Please edit that file -->

# oomph

<!-- badges: start -->

![](https://img.shields.io/badge/cool-useless-green.svg)
[![R-CMD-check](https://github.com/coolbutuseless/oomph/actions/workflows/R-CMD-check.yaml/badge.svg)](https://github.com/coolbutuseless/oomph/actions/workflows/R-CMD-check.yaml)
<!-- badges: end -->

`oomph` is a packaged for fast named look-up of data in static lists and
vectors.

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
- `mph_subset(nms, x, mph)` extract named `elems` from vector/list
  object `s` using the given hash object.

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
# strings.  To reduce the possibility of hash collisions (and make look-ups
# faster), the number of hash buckets can be changed using the 'size_factor'
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mph1 <- mph_init(nms)                  # Allocate example length(nms) buckets
mph4 <- mph_init(nms, size_factor = 4) # Allocate: 4 * length(nms) buckets
```

## Compare `match()` with `mph_match()`

``` r
bench::mark(
  match(t1, nms),
  mph_match(t1, mph1),
  mph_match(t1, mph4),
  relative = TRUE
)[, 1:5] |> knitr::kable()
```

| expression          |         min |   median |   itr/sec | mem_alloc |
|:--------------------|------------:|---------:|----------:|----------:|
| match(t1, nms)      | 1221.356332 | 1188.623 |    1.0000 |       Inf |
| mph_match(t1, mph1) |    1.199886 |    1.000 | 1030.4308 |       Inf |
| mph_match(t1, mph4) |    1.000000 |    1.000 |  944.9693 |       NaN |

``` r
bench::mark(
  match(t2, nms),
  mph_match(t2, mph1),
  mph_match(t2, mph4),
  relative = TRUE
)[, 1:5] |> knitr::kable()
```

| expression          |      min |   median |  itr/sec | mem_alloc |
|:--------------------|---------:|---------:|---------:|----------:|
| match(t2, nms)      | 4530.498 | 4219.248 |    1.000 |       Inf |
| mph_match(t2, mph1) |    1.000 |    1.000 | 4153.154 |       NaN |
| mph_match(t2, mph4) |    1.000 |    1.000 | 4024.667 |       NaN |

``` r
bench::mark(
  match(t3, nms),
  mph_match(t3, mph1),
  mph_match(t3, mph4),
  relative = TRUE
)[, 1:5] |> knitr::kable()
```

| expression          |        min |     median |  itr/sec | mem_alloc |
|:--------------------|-----------:|-----------:|---------:|----------:|
| match(t3, nms)      | 699.874553 | 779.025200 |   1.0000 |  8255.679 |
| mph_match(t3, mph1) |   1.120024 |   1.111161 | 672.1334 |     1.000 |
| mph_match(t3, mph4) |   1.000000 |   1.000000 | 757.3841 |     1.000 |

``` r
bench::mark(
  match(t4, nms),
  mph_match(t4, mph1),
  mph_match(t4, mph4),
  relative = TRUE
)[, 1:5] |> knitr::kable()
```

| expression          |       min |    median |  itr/sec | mem_alloc |
|:--------------------|----------:|----------:|---------:|----------:|
| match(t4, nms)      | 66.964124 | 69.722616 |  1.00000 |  916.3399 |
| mph_match(t4, mph1) |  1.404294 |  1.424091 | 47.77987 |    1.0000 |
| mph_match(t4, mph4) |  1.000000 |  1.000000 | 67.01958 |    1.0000 |

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
| Standard R            | 1.53ms | 1.82ms |    543.995 |    3.53MB |
| \[\] and mph indexing |  3.4µs | 3.65µs | 256173.648 |    2.09KB |
| \[\] and mph indexing | 3.16µs | 3.36µs | 274864.240 |    2.09KB |
| custom mph method     | 3.85µs | 4.06µs | 239369.482 |    4.95KB |
| custom mph method     | 3.24µs | 3.44µs | 275758.824 |      848B |

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
| big_vector\[t3\]                  | 1.47ms | 1.76ms |    562.8329 |    3.53MB |
| big_vector\[mph_match(t3, mph1)\] | 2.99µs | 3.24µs | 290091.6944 |     1.7KB |
| big_vector\[mph_match(t3, mph4)\] | 2.79µs | 2.99µs | 315578.3269 |     1.7KB |
| mph_subset(t3, big_vector, mph1)  | 2.71µs | 2.91µs | 326976.6685 |  781.73KB |
| mph_subset(t3, big_vector, mph4)  | 2.38µs |  2.5µs | 383809.1788 |      448B |

## Comparison to R’s hashed look-ups in environments

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
| Standard R            |  1.66ms |  1.77ms |    552.7683 |    3.53MB |
| \[\] and mph indexing |   3.4µs |  3.65µs | 260462.2922 |    2.09KB |
| \[\] and mph indexing |   3.2µs |   3.4µs | 274476.6011 |    2.09KB |
| custom mph method     |  3.85µs |  4.06µs | 233904.9778 |      848B |
| custom mph method     |  3.24µs |  3.44µs | 277896.9510 |      848B |
| R hashed environment  | 13.86µs | 14.23µs |  68266.4413 |      848B |

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

| expression        |      min |  median |    itr/sec | mem_alloc |
|:------------------|---------:|--------:|-----------:|----------:|
| mph_init(nms1k)   | 172.94µs | 182.9µs | 4341.87577 |      12KB |
| mph_init(nms10k)  |   1.74ms |   1.8ms |  236.40751 |  167.16KB |
| mph_init(nms100k) |  19.02ms |  20.8ms |   47.48566 |    1.38MB |
