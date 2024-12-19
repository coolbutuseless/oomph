

test_that("hashing members works", {
  
  vec <- colours()
  mph <- mph_init(vec)
  
  expect_identical(  
    match(vec, vec),
    mph_match(vec, mph)
  )  
  
  set.seed(1)
  vec_scramble <- sample(vec)
  
  expect_identical(  
    match(vec_scramble, vec),
    mph_match(vec_scramble, mph)
  )  
})


test_that("hashing non-members works", {

  vec <- colours()
  test <- as.character(1:1000)

  mph <- mph_init(vec)
  
  mph_match(test, mph)

  expect_identical(
    match(test, vec),
    mph_match(test, mph)
  )

})



if (FALSE) {
  
  rstring <- function() {
    N <- 10 #sample(1:20, 1)
    paste(sample(c(letters, LETTERS, 0:9), N, replace = TRUE), collapse = "")
  }
  
  
  vec <- colours()
  mph <- mph_init(vec)
  
  # set.seed(1)
  for (i in seq(10000000)) {
    rs <- rstring()
    if (!identical(match(rs, vec), mph_match(rs, mph))) {
      stop(rs)
    }
  }
  
  
  mph_match(rs, mph)
  vec[324]  
  mph_match(vec[324], mph)
  
}