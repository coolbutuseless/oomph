
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Initialise a minimal perfect hash with the given set of strings
#' 
#' @param s character vector of all possible strings. Duplicates and NAs not 
#'    allowed.  String length must be less than 255 characters.
#' @param size_factor default; 1
#' @param verbosity default: 0
#' 
#' @return mph object
#' @examples
#' mph <- mph_init(letters)
#' mph_match(c('h', 'e', 'l', 'l', 'o'), mph)
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mph_init <- function(s, size_factor = 1, verbosity = 0L) {
  
  stopifnot(exprs = {
    is.character(s)
    length(s) > 10
    length(s) < 500000
    !anyNA(s)
    max(nchar(s)) < 255
    min(nchar(s)) > 0
    !anyDuplicated(s)
  })
  
  .Call(mph_init_, s, size_factor, verbosity)
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Find the integer index of string elements 
#' 
#' @param s strings to find the index for
#' @param mph minimal perfect hash object created by \code{mph_init()}
#' @return integer vector of indicies of strings within the mph object. 
#'    NA if string is not a member of \code{mph} object.
#' @examples
#' mph <- mph_init(letters)
#' mph_match(c('h', 'e', 'l', 'l', 'o'), mph)
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mph_match <- function(s, mph) {
  .Call(mph_match_, s, mph)
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Subset an integer vector, numeric vector or list using a hashed index 
#' 
#' @param elems names of elements to extract
#' @param x Object to be subset. One of: integer vector, numeric vector or list
#' @param mph hash obbject
#' @return Subset of elements of x
#' @examples
#' mph <- mph_init(letters)
#' values <- 2^seq_along(letters)
#' mph_match(c('h', 'e', 'l', 'l', 'o'), mph)
#' mph_subset(c('h', 'e', 'l', 'l', 'o'), values, mph)
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mph_subset <- function(elems, x, mph) {
  .Call(mph_subset_, elems, x, mph)
}

