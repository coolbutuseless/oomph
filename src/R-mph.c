
#define R_NO_REMAP

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include "mph.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Finalizer for an external pointer to an array of hash buckets
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static void mph_extptr_finalizer(SEXP ptr_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Unpack the pointer
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  mph_t *mph = (mph_t *)R_ExternalPtrAddr(ptr_);
  mph_destroy(mph);

  R_ClearExternalPtr(ptr_);
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Unpack an external pointer to a C struct
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mph_t * external_ptr_to_mph(SEXP ptr_) {
  if (!Rf_inherits(ptr_, "mph")) {
    Rf_error("Expecting an 'mph' ExternalPtr");
  }
  
  mph_t *mph = TYPEOF(ptr_) != EXTPTRSXP ? NULL : (mph_t *)R_ExternalPtrAddr(ptr_);
  if (mph == NULL) {
    Rf_error("'mph' external pointer is invalid/NULL.");
  }
  
  return mph;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Initialise the hashmap
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP mph_init_(SEXP s_, SEXP size_factor_, SEXP verbosity_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Size factor
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  double size_factor = Rf_asReal(size_factor_);
  if (size_factor < 0.2 || size_factor > 100) {
    Rf_error("Bad size factor. Should be in range [0.5, 100], but got: %.1f", size_factor);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Setup the intermediate buckets
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  size_t nbuckets = (size_t)(Rf_length(s_) * size_factor);
  if (nbuckets < 1) {
    Rf_error("Hash with zero buckets not possible");
  }
  
  if (Rf_asInteger(verbosity_) > 0) {
    Rprintf("N buckets: %i\n", (int)nbuckets);
  }
  
  mph_t *mph = mph_init(nbuckets);
  if (mph == NULL) {
    Rf_error("mph_init_(): Couldn't initialise hashmap");
  }
 
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Bucket all the strings
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  for (int i = 0; i < Rf_length(s_); ++i) {
    if (STRING_ELT(s_, i) == NA_STRING) {
      Rf_error("mph_init_(): Cannot add NAs to hashmap");
    }
    const char *s = CHAR(STRING_ELT(s_, i));
    
    // Define the key
    uint8_t *key = (uint8_t *)s;
    size_t len   = (size_t)strlen(s);
    
    if (!mph_add(mph, key, len)) {
      Rf_error("mph_init_(): Error adding item %i: '%s'", i, s);
    }
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Dump buckets
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (Rf_asInteger(verbosity_) > 1) {
    for (int i = 0; i < mph->nbuckets; ++i) {
      Rprintf("[%3i] %3i\n", i, mph->bucket[i].value);
    }
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create an external pointer
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP ptr_ = PROTECT(R_MakeExternalPtr(mph, R_NilValue, R_NilValue));
  R_RegisterCFinalizer(ptr_, mph_extptr_finalizer);
  Rf_setAttrib(ptr_, R_ClassSymbol, Rf_mkString("mph"));
  UNPROTECT(1);
  return ptr_;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Hashmap match
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP mph_match_(SEXP s_, SEXP mph_) {
  
  mph_t *mph = external_ptr_to_mph(mph_);

  SEXP res_ = PROTECT(Rf_allocVector(INTSXP, Rf_length(s_)));
  int *res = INTEGER(res_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // For each string, find its index. 
  // If 0, then string was not found
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  for (int i = 0; i < Rf_length(s_); ++i) {
    const char *s = CHAR(STRING_ELT(s_, i));
    
    int idx = mph_lookup(mph, (uint8_t *)s, strlen(s));
    res[i] = idx < 0 ? NA_INTEGER : idx + 1;
  }
  
  UNPROTECT(1);
  return res_;
}




