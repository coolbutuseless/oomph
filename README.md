
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
t0  <- sample(nms,    1, replace = TRUE)
t1  <- sample(nms,   10, replace = TRUE)
t2  <- sample(nms,  100, replace = TRUE)
t3  <- sample(nms, 1000, replace = TRUE)

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
  match(t0, nms),
  mph_match(t0, mph),
  check = FALSE
)[, 1:5] |> knitr::kable()
```

| expression         |   min |   median |      itr/sec | mem_alloc |
|:-------------------|------:|---------:|-------------:|----------:|
| match(t0, nms)     | 878µs |   1.13ms |     865.4985 |    3.82MB |
| mph_match(t0, mph) | 246ns | 328.06ns | 2359452.7544 |        0B |

``` r
bench::mark(
  match(t1, nms),
  mph_match(t1, mph),
  check = FALSE
)[, 1:5] |> knitr::kable()
```

| expression         |      min |   median |      itr/sec | mem_alloc |
|:-------------------|---------:|---------:|-------------:|----------:|
| match(t1, nms)     |   5.69ms |   6.16ms |     160.4282 |    7.82MB |
| mph_match(t1, mph) | 369.04ns | 450.99ns | 2021899.3830 |        0B |

``` r
bench::mark(
  match(t2, nms),
  mph_match(t2, mph),
  check = FALSE
)[, 1:5] |> knitr::kable()
```

| expression         |    min | median |     itr/sec | mem_alloc |
|:-------------------|-------:|-------:|------------:|----------:|
| match(t2, nms)     | 5.41ms | 5.68ms |    176.5015 |    7.82MB |
| mph_match(t2, mph) | 2.42µs | 2.58µs | 379750.3944 |      448B |

``` r
bench::mark(
  match(t3, nms),
  mph_match(t3, mph),
  check = FALSE
)[, 1:5] |> knitr::kable()
```

| expression         |     min | median |    itr/sec | mem_alloc |
|:-------------------|--------:|-------:|-----------:|----------:|
| match(t3, nms)     |  5.67ms |  5.9ms |   168.2901 |    7.83MB |
| mph_match(t3, mph) | 26.57µs | 27.7µs | 34571.8071 |    3.95KB |

### Vector subsetting - Extract 100 elements of a `vector` by name

``` r
bench::mark(
  big_vector[t2],
  big_vector[mph_match(t2, mph)],
  check = FALSE
)[, 1:5] |> knitr::kable()
```

| expression                       |    min | median |     itr/sec | mem_alloc |
|:---------------------------------|-------:|-------:|------------:|----------:|
| big_vector\[t2\]                 | 6.33ms | 6.74ms |    148.1325 |    7.82MB |
| big_vector\[mph_match(t2, mph)\] | 3.16µs | 3.48µs | 254227.5593 |     1.7KB |

### List subsetting - Extract 100 elements of a `list` by name

Also compare to using hashed named lookup in a standard R environment

``` r
ee <- as.environment(big_list)

bench::mark(
  `Standard R`           = big_list[t2],
  `R hashed environment` = mget(t2, ee),
  `[] and mph indexing`  = big_list[mph_match(t2, mph)],
  check = FALSE
)[, 1:5] |> knitr::kable()
```

| expression            |     min |  median |     itr/sec | mem_alloc |
|:----------------------|--------:|--------:|------------:|----------:|
| Standard R            |  6.01ms |  6.17ms |    160.7191 |    7.82MB |
| R hashed environment  | 18.66µs | 18.86µs |  52150.0227 |      848B |
| \[\] and mph indexing |  3.57µs |  3.81µs | 247564.5570 |    2.09KB |

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
  check = FALSE
)[, 1:5] |> knitr::kable()
```

| expression        |      min |   median |    itr/sec | mem_alloc |
|:------------------|---------:|---------:|-----------:|----------:|
| mph_init(nms1k)   |  74.13µs |  80.56µs | 9257.35214 |      12KB |
| mph_init(nms10k)  | 658.75µs | 745.85µs |  748.92179 |  167.16KB |
| mph_init(nms100k) |   8.91ms |   9.45ms |   78.85602 |    1.38MB |
