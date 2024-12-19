
<!-- README.md is generated from README.Rmd. Please edit that file -->

# oomph

<!-- badges: start -->

![](https://img.shields.io/badge/cool-useless-green.svg)
[![R-CMD-check](https://github.com/coolbutuseless/oomph/actions/workflows/R-CMD-check.yaml/badge.svg)](https://github.com/coolbutuseless/oomph/actions/workflows/R-CMD-check.yaml)
<!-- badges: end -->

`oomph` is a technical demonstration of using a *minimal perfect hashing
function* to do fast named lookups of data in lists and vectors.

The hashed lookup can be **1000x** faster than R’s standard lookup
method (depending on number of elements to extract).

Next steps:

- This would be much better with a dynamic minimal perfect hash which
  updated with new elements
- The included mph code has some issues with large numbers of input
  strings. I think it may have something to do with large stack using
  with the recursive calling of `graph.c / cyclicdeledge()`

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

| expression         |     min |   median |  itr/sec | mem_alloc |
|:-------------------|--------:|---------:|---------:|----------:|
| match(t1, nms)     | 1020.06 | 1069.187 |   1.0000 |  394.4094 |
| mph_match(t1, mph) |    1.00 |    1.000 | 923.3507 |    1.0000 |

``` r
bench::mark(
  match(t2, nms),
  mph_match(t2, mph),
  relative = TRUE
)[, 1:5] |> knitr::kable()
```

| expression         |      min |   median |  itr/sec | mem_alloc |
|:-------------------|---------:|---------:|---------:|----------:|
| match(t2, nms)     | 3909.914 | 3725.635 |    1.000 |       Inf |
| mph_match(t2, mph) |    1.000 |    1.000 | 3710.261 |       NaN |

``` r
bench::mark(
  match(t3, nms),
  mph_match(t3, mph),
  relative = TRUE
)[, 1:5] |> knitr::kable()
```

| expression         |      min |   median |  itr/sec | mem_alloc |
|:-------------------|---------:|---------:|---------:|----------:|
| match(t3, nms)     | 610.9976 | 656.6756 |   1.0000 |  8255.679 |
| mph_match(t3, mph) |   1.0000 |   1.0000 | 643.6448 |     1.000 |

``` r
bench::mark(
  match(t4, nms),
  mph_match(t4, mph),
  relative = TRUE
)[, 1:5] |> knitr::kable()
```

| expression         |      min |   median |  itr/sec | mem_alloc |
|:-------------------|---------:|---------:|---------:|----------:|
| match(t4, nms)     | 55.27912 | 61.32186 |  1.00000 |  916.3399 |
| mph_match(t4, mph) |  1.00000 |  1.00000 | 59.49752 |    1.0000 |

## List subsetting - Extract 100 elements of a `list` by name

``` r
bench::mark(
  `Standard R`           = big_list[t3],
  `[] and mph indexing`  = big_list[mph_match(t3, mph)],
  `custom mph method`    = mph_subset(big_list, t3, mph),
  check = FALSE
)[, 1:5] |> knitr::kable()
```

| expression            |    min | median |    itr/sec | mem_alloc |
|:----------------------|-------:|-------:|-----------:|----------:|
| Standard R            | 1.63ms | 1.84ms |    531.951 |    3.53MB |
| \[\] and mph indexing |  3.9µs | 4.14µs | 227241.388 |    2.09KB |
| custom mph method     | 3.48µs | 3.69µs | 254917.586 |    4.95KB |

## Vector subsetting - Extract 100 elements of a `vector` by name

``` r
bench::mark(
  big_vector[t3],
  big_vector[mph_match(t3, mph)],
  mph_subset(big_vector, t3, mph),
  check = F
)[, 1:5] |> knitr::kable()
```

| expression                       |    min | median |     itr/sec | mem_alloc |
|:---------------------------------|-------:|-------:|------------:|----------:|
| big_vector\[t3\]                 | 1.63ms | 1.81ms |    544.4496 |    3.53MB |
| big_vector\[mph_match(t3, mph)\] | 3.44µs | 3.69µs | 256396.5572 |     1.7KB |
| mph_subset(big_vector, t3, mph)  | 2.79µs | 2.95µs | 321624.1745 |  781.73KB |

## Creating the C code for the minimal perfect hash if using ‘mph’ on the command line.

There are a number of programs which will produce C code to do minimal
perfect hashing of a list of strings e.g. `gperf`, `cmph`.

I chose `mph` as described in [A Family of Perfect Hashing
Methods(pdf)](https://staff.itee.uq.edu.au/havas/TR0242.pdf)

- Download and compile
  \[<http://www.ibiblio.org/pub/Linux/devel/lang/c/mph-1.2.tar.gz>\]
- In R: `writeLines(colors(), "input.txt")`
- On the command line: `mph < input.txt | emitc > hash.c`
- Incorporate the `hash.c` code into this package and add the
  `col_to_rgb()` wrapper which uses it.
