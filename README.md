
<!-- README.md is generated from README.Rmd. Please edit that file -->

# oomph

<!-- badges: start -->

![](https://img.shields.io/badge/cool-useless-green.svg)
[![R-CMD-check](https://github.com/coolbutuseless/oomph/actions/workflows/R-CMD-check.yaml/badge.svg)](https://github.com/coolbutuseless/oomph/actions/workflows/R-CMD-check.yaml)
<!-- badges: end -->

`oomph` is a technical demonstration of using a hash for fast named
lookups of data in lists and vectors.

Internally this is just a hashmap in C which maps strings to integers.
To R, this appears as a minimal perfect hash where each string maps to
its index.

The hashed lookup can be **1000x** faster than R’s standard lookup
method (depending on number of elements in original object and the
number of elements to extract).

Next steps:

- This would be much better with a dynamic hash which could be easily
  updated with new elements. Currently if even a single name was
  added/removed or changes, then the entire hash would need to be
  recalculated.

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
# strings.  To reduce the possibility of hash collisions (and make lookups
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

| expression          |         min |   median |  itr/sec | mem_alloc |
|:--------------------|------------:|---------:|---------:|----------:|
| match(t1, nms)      | 1219.756388 | 1192.052 |   1.0000 |       Inf |
| mph_match(t1, mph1) |    1.000000 |    1.000 | 921.1321 |       Inf |
| mph_match(t1, mph4) |    1.199886 |    1.000 | 898.2117 |       NaN |

``` r
bench::mark(
  match(t2, nms),
  mph_match(t2, mph1),
  mph_match(t2, mph4),
  relative = TRUE
)[, 1:5] |> knitr::kable()
```

| expression          |        min |   median |  itr/sec | mem_alloc |
|:--------------------|-----------:|---------:|---------:|----------:|
| match(t2, nms)      | 5643.85358 | 4277.996 |    1.000 |       Inf |
| mph_match(t2, mph1) |    1.14277 |    1.000 | 4051.918 |       NaN |
| mph_match(t2, mph4) |    1.00000 |    1.000 | 4005.527 |       NaN |

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
| match(t3, nms)      | 780.354070 | 812.616613 |   1.0000 |  8255.679 |
| mph_match(t3, mph1) |   1.119992 |   1.150912 | 692.7109 |     1.000 |
| mph_match(t3, mph4) |   1.000000 |   1.000000 | 757.7391 |     1.000 |

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
| match(t4, nms)      | 68.517174 | 76.114719 |  1.00000 |  916.3399 |
| mph_match(t4, mph1) |  1.405749 |  1.403881 | 53.76611 |    1.0000 |
| mph_match(t4, mph4) |  1.000000 |  1.000000 | 72.01826 |    1.0000 |

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
| Standard R            |  1.6ms | 1.77ms |    555.6639 |    3.53MB |
| \[\] and mph indexing |  3.4µs | 3.69µs | 254054.8837 |    2.09KB |
| \[\] and mph indexing |  3.2µs |  3.4µs | 271543.3681 |    2.09KB |
| custom mph method     | 3.81µs | 4.02µs | 238153.1225 |    4.95KB |
| custom mph method     | 3.16µs | 3.36µs | 278159.6465 |      848B |

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
| big_vector\[t3\]                  | 1.54ms | 1.78ms |    552.4683 |    3.53MB |
| big_vector\[mph_match(t3, mph1)\] | 3.03µs | 3.28µs | 288699.7940 |     1.7KB |
| big_vector\[mph_match(t3, mph4)\] | 2.79µs | 2.95µs | 316469.7965 |     1.7KB |
| mph_subset(t3, big_vector, mph1)  | 2.71µs | 2.87µs | 325130.2797 |  781.73KB |
| mph_subset(t3, big_vector, mph4)  | 2.34µs | 2.46µs | 379264.2628 |      448B |
