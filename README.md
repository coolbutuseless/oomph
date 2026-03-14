
<!-- README.md is generated from README.Rmd. Please edit that file -->

# oomph

<!-- badges: start -->

![](https://img.shields.io/badge/cool-useless-green.svg)
[![R-CMD-check](https://github.com/coolbutuseless/oomph/actions/workflows/R-CMD-check.yaml/badge.svg)](https://github.com/coolbutuseless/oomph/actions/workflows/R-CMD-check.yaml)
<!-- badges: end -->

`oomph` is a package for fast string matching within a static set of
strings - where each string maps to its integer position.

This is useful for fast named look-up in fixed lists and vectors.

Internally this uses a hashmap written in C to map strings to integers.
In R, this appears as a minimal perfect hash where each string maps to
its position in the original data (unknown strings will return `NA`).

The hashed look-up can be more than **1000x** faster than R’s standard
look-up method (depending on number of elements in original object and
the number of elements to extract).

## What’s in the box

- `mph <- mph_init(s)` initialise a hashmap with the given set of
  strings
- `mph_match(s, mph)` find the indices of the strings `s` (equivalent to
  R’s `match()`)

## Installation

You can install from [GitHub](https://github.com/coolbutuseless/oomph)
with:

``` r
# install.package('remotes')
remotes::install_github('coolbutuseless/oomph')
```

## Simple example

``` r
mph <- mph_init(c('mary', 'had', 'a', 'little', 'lamb'))
mph_match('mary', mph)
```

    #> [1] 1

``` r
mph_match('lamb', mph)
```

    #> [1] 5

``` r
mph_match('monkey', mph) # not a string from the original vector.
```

    #> [1] NA

## Benchmarking

### Setup benchmark data

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
t2  <- sample(nms,   100, replace = TRUE)
t3  <- sample(nms,  1000, replace = TRUE)
t4  <- sample(nms, 10000, replace = TRUE)

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# By default, the number of hashmap buckets is twice the number of 
# strings.  To reduce the possibility of hash collisions (and possibly make look-ups
# faster), the number of hashmap buckets can be increased by increasing 'size_factor'
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mph <- mph_init(nms) 
```

### Compare base R `match()` with oomph’s `mph_match()`

``` r
bench::mark(
  match(t2, nms),
  mph_match(t2, mph),
  check = TRUE
)[, 1:5] |> knitr::kable()
```

| expression         |    min | median |     itr/sec | mem_alloc |
|:-------------------|-------:|-------:|------------:|----------:|
| match(t2, nms)     | 4.99ms | 5.99ms |    165.7736 |    7.82MB |
| mph_match(t2, mph) | 2.38µs | 3.03µs | 322519.8667 |      448B |

``` r
bench::mark(
  match(t3, nms),
  mph_match(t3, mph),
  check = TRUE
)[, 1:5] |> knitr::kable()
```

| expression         |     min |  median |    itr/sec | mem_alloc |
|:-------------------|--------:|--------:|-----------:|----------:|
| match(t3, nms)     |  5.45ms |  6.02ms |   165.9522 |    7.83MB |
| mph_match(t3, mph) | 26.73µs | 30.91µs | 31813.0245 |    3.95KB |

``` r
bench::mark(
  match(t4, nms),
  mph_match(t4, mph),
  check = TRUE
)[, 1:5] |> knitr::kable()
```

| expression         |      min |   median |   itr/sec | mem_alloc |
|:-------------------|---------:|---------:|----------:|----------:|
| match(t4, nms)     |   5.25ms |   5.39ms |  182.1261 |    7.93MB |
| mph_match(t4, mph) | 307.21µs | 330.67µs | 2916.0853 |   39.11KB |

### Vector subsetting - Extract 100 elements of a `vector` by name

``` r
bench::mark(
  big_vector[t2],
  big_vector[mph_match(t2, mph)],
  check = TRUE
)[, 1:5] |> knitr::kable()
```

| expression                       |    min | median |     itr/sec | mem_alloc |
|:---------------------------------|-------:|-------:|------------:|----------:|
| big_vector\[t2\]                 |  5.1ms | 6.04ms |    165.9051 |    7.82MB |
| big_vector\[mph_match(t2, mph)\] | 3.12µs |  4.1µs | 227335.4983 |     1.7KB |

### List subsetting - Extract 100 elements of a `list` by name

Also compare to using hashed named lookup in a standard R environment

``` r
ee <- as.environment(big_list)

bench::mark(
  `Standard R`           = big_list[t2],
  `R hashed environment` = mget(t2, ee),
  `[] and mph indexing`  = big_list[mph_match(t2, mph)],
  check = TRUE
)[, 1:5] |> knitr::kable()
```

| expression            |     min |  median |     itr/sec | mem_alloc |
|:----------------------|--------:|--------:|------------:|----------:|
| Standard R            |  5.51ms |  5.64ms |    174.4341 |    7.82MB |
| R hashed environment  | 18.33µs | 18.82µs |  51150.7710 |      848B |
| \[\] and mph indexing |  3.65µs |  3.94µs | 229851.9970 |    2.09KB |

### Factor creation

Note: this is not a direct replacement for `as.factor()` as levels are
in order of first seen, not alphabetical.

``` r
v <- c('h', 'e', 'l', 'l', 'o')
mph_as_factor(v)
```

    #> [1] h e l l o
    #> Levels: h e l o

``` r
as.factor(v)
```

    #> [1] h e l l o
    #> Levels: e h l o

``` r
bench::mark(
  as.factor(t4),
  mph_as_factor(t4),
  check = FALSE
)[, 1:5] |> knitr::kable()
```

| expression        |     min |  median |   itr/sec | mem_alloc |
|:------------------|--------:|--------:|----------:|----------:|
| as.factor(t4)     | 13.89ms | 14.69ms |  67.97609 |    1.33MB |
| mph_as_factor(t4) |  1.16ms |  1.33ms | 720.43071 |  116.42KB |

### Time taken to build the hashmap

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
  check = TRUE
)[, 1:5] |> knitr::kable()
```

| expression        |      min |   median |    itr/sec | mem_alloc |
|:------------------|---------:|---------:|-----------:|----------:|
| mph_init(nms1k)   |  42.97µs |  51.33µs | 19109.1432 |        0B |
| mph_init(nms10k)  | 458.83µs | 542.92µs |  1840.9348 |        0B |
| mph_init(nms100k) |   5.33ms |   5.85ms |   165.4952 |        0B |
