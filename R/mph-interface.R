
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Initialise a minimal perfect hash with the given set of strings
#' 
#' @param s character vector of all possible strings. Duplicates and NAs not 
#'    allowed.  String length must be less than 255 characters.
#' @return mph object
#' @examples
#' mph <- mph_init(letters)
#' mph_match(c('h', 'e', 'l', 'l', 'o'), mph)
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mph_init <- function(s) {
  
  stopifnot(exprs = {
    is.character(s)
    length(s) > 10
    length(s) < 500000
    !anyNA(s)
    max(nchar(s)) < 255
    min(nchar(s)) > 0
    !anyDuplicated(s)
  })
  
  .Call(mph_init_, s)
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
  .Call(mph_hash_, mph, s)
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Subset an integer vector, numeric vector or list using a hashed index 
#' 
#' @param x integer vector, numeric vector or list
#' @param s strings to search for
#' @param mph minimal perfect hash of string population
#' @return Subset of elements of x
#' @examples
#' mph <- mph_init(letters)
#' value <- 2^seq_along(letters)
#' mph_match(c('h', 'e', 'l', 'l', 'o'), mph)
#' mph_subset(value, c('h', 'e', 'l', 'l', 'o'), mph)
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mph_subset <- function(x, s, mph) {
  .Call(mph_subset_, x, mph, s)
}

