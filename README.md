
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
mph <- mph_init(nms) # Allocate exactly length(nms) buckets
```

## Compare `match()` with `mph_match()`

``` r
bench::mark(
  match(t1, nms),
  mph_match(t1, mph)
)[, 1:5] |> knitr::kable()
```

| expression         |   min | median |     itr/sec | mem_alloc |
|:-------------------|------:|-------:|------------:|----------:|
| match(t1, nms)     | 251µs |  347µs |    2862.847 |    1.53MB |
| mph_match(t1, mph) | 246ns |  328ns | 2591169.701 |    3.97KB |

``` r
bench::mark(
  match(t2, nms),
  mph_match(t2, mph)
)[, 1:5] |> knitr::kable()
```

| expression         |      min |   median |     itr/sec | mem_alloc |
|:-------------------|---------:|---------:|------------:|----------:|
| match(t2, nms)     |   1.64ms |   1.81ms |     545.109 |    3.53MB |
| mph_match(t2, mph) | 369.04ns | 451.05ns | 2037829.640 |        0B |

``` r
bench::mark(
  match(t3, nms),
  mph_match(t3, mph)
)[, 1:5] |> knitr::kable()
```

| expression         |    min | median |     itr/sec | mem_alloc |
|:-------------------|-------:|-------:|------------:|----------:|
| match(t3, nms)     | 1.61ms | 1.81ms |    546.2491 |    3.53MB |
| mph_match(t3, mph) | 2.62µs | 2.83µs | 338024.7303 |      448B |

``` r
bench::mark(
  match(t4, nms),
  mph_match(t4, mph)
)[, 1:5] |> knitr::kable()
```

| expression         |     min |  median |   itr/sec | mem_alloc |
|:-------------------|--------:|--------:|----------:|----------:|
| match(t4, nms)     |  1.58ms |  1.81ms |   544.457 |    3.54MB |
| mph_match(t4, mph) | 34.81µs | 35.22µs | 27602.687 |    3.95KB |

## Vector subsetting - Extract 100 elements of a `vector` by name

``` r
bench::mark(
  big_vector[t3],
  big_vector[mph_match(t3, mph)],
  check = F
)[, 1:5] |> knitr::kable()
```

| expression                       |    min | median |     itr/sec | mem_alloc |
|:---------------------------------|-------:|-------:|------------:|----------:|
| big_vector\[t3\]                 | 1.58ms | 1.82ms |    541.7525 |    3.53MB |
| big_vector\[mph_match(t3, mph)\] | 3.44µs | 3.69µs | 252676.8653 |     1.7KB |

## List subsetting - Extract 100 elements of a `list` by name

``` r
bench::mark(
  `Standard R`           = big_list[t3],
  `[] and mph indexing`  = big_list[mph_match(t3, mph)],
  check = FALSE
)[, 1:5] |> knitr::kable()
```

| expression            |    min | median |     itr/sec | mem_alloc |
|:----------------------|-------:|-------:|------------:|----------:|
| Standard R            | 1.59ms | 1.81ms |    541.2026 |    3.53MB |
| \[\] and mph indexing | 3.81µs | 4.06µs | 229877.9853 |    2.09KB |

## Comparison of hashed list subsetting to R’s hashed look-ups in environments

``` r
ee <- as.environment(big_list)

bench::mark(
  `Standard R`           = big_list[t3],
  `[] and mph indexing`  = big_list[mph_match(t3, mph)],
  `R hashed environment` = mget(t3, ee),
  check = FALSE
)[, 1:5] |> knitr::kable()
```

| expression            |     min |  median |     itr/sec | mem_alloc |
|:----------------------|--------:|--------:|------------:|----------:|
| Standard R            |  1.58ms |  1.82ms |    539.8538 |    3.53MB |
| \[\] and mph indexing |  3.73µs |  4.06µs | 230860.2435 |    2.09KB |
| R hashed environment  | 13.86µs | 14.31µs |  67753.0814 |      848B |

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
| mph_init(nms1k)   | 174.41µs | 186.51µs | 5168.24470 |      12KB |
| mph_init(nms10k)  |   1.74ms |   1.83ms |  221.74792 |  167.16KB |
| mph_init(nms100k) |  18.85ms |  21.43ms |   46.61468 |    1.38MB |

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
| baseR          | 152.7µs | 179.9µs |  5193.685 |   558.3KB |
| oomph          |  35.9µs |  37.4µs | 25811.219 |      20KB |
| oomph + insitu |  29.2µs |  29.9µs | 32263.890 |    14.5KB |

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
| baseR          | 65.59ms | 70.91ms |  13.68771 |   134.2MB |
| oomph          |   1.4ms |  1.56ms | 349.89269 |    20.2KB |
| oomph + insitu |  1.39ms |  1.47ms | 572.74436 |    11.3KB |
