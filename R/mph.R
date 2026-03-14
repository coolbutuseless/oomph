

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Initialise a hash object with the given set of strings
#' 
#' @param s character vector. NAs must not be present. If duplicates are present
#'        then \code{mph_match()} will only return the index of the first occurrence.
#' @param size_factor Adjusts the internal number of hash slots. Default: 2. Increasing 
#'    this factor lowers the chance of hash collisions and may speed up the
#'    overall performance.
#' @param verbosity What level of debugging information should be output? Default: 0
#'    (no debugging output). Valid levels: 0, 1, 2
#' 
#' @return Hash object
#' @examples
#' mph <- mph_init(letters)
#' mph_match(c('h', 'e', 'l', 'l', 'o'), mph)
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mph_init <- function(s, size_factor = 2, verbosity = 0L) {
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
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mph_match <- function(s, mph) {
  .Call(mph_match_, s, mph)
}



#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#' Convert string vector to a factor
#' 
#' Factor levels are in "first seen" order i.e. not sorted alphabetically.
#' 
#' @param s strings
#' @return factor
#' @examples
#' mph_as_factor(letters)
#' mph_as_factor(c('h', 'e', 'l', 'l', 'o'))
#' @export
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mph_as_factor <- function(s) {
  .Call(mph_as_factor_, s)
}

