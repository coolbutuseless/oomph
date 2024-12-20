
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Initialise a hash object with the given set of strings
#' 
#' @param s character vector of all possible strings. Duplicates and NAs are not 
#'    allowed.
#' @param size_factor Adjusts the internal number of hash slots. Default: 1. Increasing 
#'    this factor lowers the chance of hash collisions and may speed up the
#'    overall performance.
#' @param verbosity Default: 0
#' 
#' @return Hash object
#' @examples
#' mph <- mph_init(letters)
#' mph_match(c('h', 'e', 'l', 'l', 'o'), mph)
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mph_init <- function(s, size_factor = 1, verbosity = 0L) {
  
  stopifnot(exprs = {
    is.character(s)
    !anyNA(s)
    min(nchar(s)) > 0
    !anyDuplicated(s)
  })
  
  .Call(mph_init_, s, size_factor, verbosity)
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Find the integer index of string elements.
#' 
#' This is equivalent to R's \code{match()} function.
#' 
#' @param s strings
#' @param mph hash object created by \code{mph_init()}
#' @return integer vector of indexes for each of the strings. 
#'    NA if string is not a member.
#' @examples
#' mph <- mph_init(letters)
#' mph_match(c('h', 'e', 'l', 'l', 'o'), mph)
#' match(c('h', 'e', 'l', 'l', 'o'), letters)
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mph_match <- function(s, mph) {
  .Call(mph_match_, s, mph)
}

