
<!-- README.md is generated from README.Rmd. Please edit that file -->

# oomph

<!-- badges: start -->

![](https://img.shields.io/badge/cool-useless-green.svg)
[![R-CMD-check](https://github.com/coolbutuseless/oomph/actions/workflows/R-CMD-check.yaml/badge.svg)](https://github.com/coolbutuseless/oomph/actions/workflows/R-CMD-check.yaml)
<!-- badges: end -->

`oomph` is a technical demonstration of using a *hashmap* to do fast
named lookups of data in lists and vectors.

The hashed lookup can be **1000x** faster than R’s standard lookup
method (depending on number of elements in original object and the
number of elements to extract).

Next steps:

- This would be much better with a dynamic hash which could be easily
  updated with new elements

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
# 
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mph <- mph_init(nms)
```

## Compare `match()` with `mph_match()`

``` r
bench::mark(
  match(t1, nms),
  mph_match(t1, mph),
  relative = TRUE
)[, 1:5] |> knitr::kable()
```

| expression         |      min |   median |  itr/sec | mem_alloc |
|:-------------------|---------:|---------:|---------:|----------:|
| match(t1, nms)     | 1219.156 | 1221.335 |   1.0000 |  394.4094 |
| mph_match(t1, mph) |    1.000 |    1.000 | 956.1839 |    1.0000 |

``` r
bench::mark(
  match(t2, nms),
  mph_match(t2, mph),
  relative = TRUE
)[, 1:5] |> knitr::kable()
```

| expression         |      min |   median |  itr/sec | mem_alloc |
|:-------------------|---------:|---------:|---------:|----------:|
| match(t2, nms)     | 4675.873 | 4311.395 |    1.000 |       Inf |
| mph_match(t2, mph) |    1.000 |    1.000 | 4055.791 |       NaN |

``` r
bench::mark(
  match(t3, nms),
  mph_match(t3, mph),
  relative = TRUE
)[, 1:5] |> knitr::kable()
```

| expression         |      min |   median |  itr/sec | mem_alloc |
|:-------------------|---------:|---------:|---------:|----------:|
| match(t3, nms)     | 643.6262 | 709.1614 |   1.0000 |  8255.679 |
| mph_match(t3, mph) |   1.0000 |   1.0000 | 683.6333 |     1.000 |

``` r
bench::mark(
  match(t4, nms),
  mph_match(t4, mph),
  relative = TRUE
)[, 1:5] |> knitr::kable()
```

| expression         |      min |   median |  itr/sec | mem_alloc |
|:-------------------|---------:|---------:|---------:|----------:|
| match(t4, nms)     | 51.78675 | 54.70721 |  1.00000 |  916.3399 |
| mph_match(t4, mph) |  1.00000 |  1.00000 | 54.09437 |    1.0000 |

## List subsetting - Extract 100 elements of a `list` by name

``` r
bench::mark(
  `Standard R`           = big_list[t3],
  `[] and mph indexing`  = big_list[mph_match(t3, mph)],
  `custom mph method`    = mph_subset(t3, big_list, mph),
  check = FALSE
)[, 1:5] |> knitr::kable()
```

| expression            |    min | median |     itr/sec | mem_alloc |
|:----------------------|-------:|-------:|------------:|----------:|
| Standard R            | 1.55ms | 1.78ms |    551.1629 |    3.53MB |
| \[\] and mph indexing | 3.44µs | 3.69µs | 246339.6714 |    2.09KB |
| custom mph method     | 3.85µs | 4.06µs | 231660.9440 |    4.95KB |

## Vector subsetting - Extract 100 elements of a `vector` by name

``` r
bench::mark(
  big_vector[t3],
  big_vector[mph_match(t3, mph)],
  mph_subset(t3, big_vector, mph),
  check = F
)[, 1:5] |> knitr::kable()
```

| expression                       |    min | median |     itr/sec | mem_alloc |
|:---------------------------------|-------:|-------:|------------:|----------:|
| big_vector\[t3\]                 | 1.62ms | 1.76ms |    554.0347 |    3.53MB |
| big_vector\[mph_match(t3, mph)\] | 3.08µs | 3.32µs | 284129.7080 |     1.7KB |
| mph_subset(t3, big_vector, mph)  | 2.79µs | 2.95µs | 323627.9774 |  781.73KB |
