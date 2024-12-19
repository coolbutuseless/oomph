
#define R_NO_REMAP

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mph.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Unpack an external pointer to a C struct
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mphash_struct * external_ptr_to_mphash(SEXP ptr_) {
  if (!Rf_inherits(ptr_, "mphash")) {
    Rf_error("Expecting an 'mphash' ExternalPtr");
  }
  
  mphash_struct *mphash = TYPEOF(ptr_) != EXTPTRSXP ? NULL : (mphash_struct *)R_ExternalPtrAddr(ptr_);
  if (mphash == NULL) {
    Rf_error("mphash pointer is invalid/NULL.");
  }
  
  return mphash;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Finalizer for an 'rctx struct' object.
//
// This function will be called when portaudio stream object gets 
// garbage collected.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static void mphash_extptr_finalizer(SEXP ptr_) {
  
  // Rprintf("Finialsing mphash pointer\n");
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Unpack the pointer
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  mphash_struct *mphash = (mphash_struct *)R_ExternalPtrAddr(ptr_);
  
  if (mphash != NULL) {
    memfree(mphash);
    free(mphash);
  }
  
  R_ClearExternalPtr(ptr_);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read keys from an R character vector
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Keys *keynew_rvec(SEXP vec_) {
  Keys *kp;
  
  kp          = xmalloc(sizeof * kp);
  kp->ap      = arnew();
  kp->keys    = kp->kmax = kp->kptr = 0;
  kp->maxchar = 0;
  kp->minchar = KEYMAXCHAR;
  kp->minlen  = KEYMAXLEN;
  kp->maxlen  = 0;
  
  for (int i = 0; i < Rf_length(vec_); i++) {
    const char *val = CHAR(STRING_ELT(vec_, i));
    savekey_array(kp, (char *)val);
  }
  
  return kp;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read the keys
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void readkeys_rvec(SEXP vec_, mphash_struct *mphash, state_struct *state, options *opts) {
  mphash->kp   = keynew_rvec(vec_);
  mphash->m    = keynum(mphash->kp);
  mphash->n    = ceil(mphash->c * mphash->m);
  opts->maxlen = min(mphash->kp->maxlen, opts->maxlen);
  opts->tsize  = opts->maxlen * ALPHASZ;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Initialise a minimal-perfect-hash object
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP mph_init_(SEXP vec_) {
  // Rprintf("mph_init()\n");
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Options Stuct
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  options opts = {
    .maxlen = INT_MAX,
    .seed = 0,
    .emitbinary = 1,
    .loop = 0
  };
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Running State Struct
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  state_struct state;
  
  
  mphash_struct *mphash = (mphash_struct *)calloc(1, sizeof(mphash_struct));
  if (mphash == NULL) {
    Rf_error("Allocation error in mph_init_()");
  }
  mphash->d = 3;
  mphash->c = 1.23;
  
  
  readkeys_rvec(vec_, mphash, &state, &opts);
  
  memalloc(mphash, &opts);
  
  map(mphash, &state, &opts);
  assign(mphash);
  verify(mphash, &opts);
  calculate_g(mphash);
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // key dump
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Rprintf("Key 0: %s\n", mphash->kp->keys[0]);
  // Rprintf("Key 1: %s\n", mphash->kp->keys[1]);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Store char vec
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  mphash->nkeys = Rf_length(vec_);
  
  
  SEXP ptr_ = R_MakeExternalPtr(mphash, R_NilValue, R_NilValue);
  PROTECT(ptr_);
  R_RegisterCFinalizer(ptr_, mphash_extptr_finalizer);
  Rf_setAttrib(ptr_, R_ClassSymbol, Rf_mkString("mphash"));
  UNPROTECT(1);
  
  
  return ptr_;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Lookup the values of 'vec' to find their index in 'mphash'
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP mph_hash_(SEXP mphash_, SEXP vec_) {
  
  mphash_struct *mphash = external_ptr_to_mphash(mphash_);
  
  SEXP res_ = PROTECT(Rf_allocVector(INTSXP, Rf_length(vec_)));
  int *ptr = INTEGER(res_);
  
  for (int i = 0; i < Rf_length(vec_); i++) {
    const char *val = CHAR(STRING_ELT(vec_, i));
    int idx = hash(mphash, val);
    ptr[i] = idx < 0 ? NA_INTEGER : idx + 1;
  }
  
  UNPROTECT(1);
  return res_;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Subset
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP mph_subset_(SEXP x_, SEXP mphash_, SEXP vec_) {
  
  mphash_struct *mphash = TYPEOF(mphash_) != EXTPTRSXP ? NULL : (mphash_struct *)R_ExternalPtrAddr(mphash_);
  if (mphash == NULL) {
    Rf_error("mphash pointer is invalid/NULL.");
  }
  
  if (Rf_length(x_) != mphash->nkeys) {
    Rf_error("Length(x) = %i does not match number of keys %i", 
             (int)Rf_length(x_), mphash->nkeys);
  }
  
  
  SEXP res_ = R_NilValue;
  
  if (TYPEOF(x_) == VECSXP) {
    res_ = PROTECT(Rf_allocVector(VECSXP, Rf_length(vec_)));

    for (int i = 0; i < Rf_length(vec_); i++) {
      const char *val = CHAR(STRING_ELT(vec_, i));
      int idx = hash(mphash, val);
      if (idx < 0) {
        SET_VECTOR_ELT(res_, i, R_NilValue);
      } else {
        SET_VECTOR_ELT(res_, i, VECTOR_ELT(x_, idx));
      }
    }
    
  } else if (Rf_isInteger(x_)) {
    int *xptr = INTEGER(x_);
    res_ = PROTECT(Rf_allocVector(INTSXP, Rf_length(vec_)));
    int *ptr = INTEGER(res_);
    for (int i = 0; i < Rf_length(vec_); i++) {
      const char *val = CHAR(STRING_ELT(vec_, i));
      int idx = hash(mphash, val);
      if (idx < 0) {
        ptr[i] = NA_INTEGER;
      } else {
        ptr[i] = xptr[idx];
      }
    }
  } else if (Rf_isReal(x_)) {
    double *xptr = REAL(x_);
    res_ = PROTECT(Rf_allocVector(REALSXP, Rf_length(vec_)));
    double *ptr = REAL(res_);
    for (int i = 0; i < Rf_length(vec_); i++) {
      const char *val = CHAR(STRING_ELT(vec_, i));
      int idx = hash(mphash, val);
      if (idx < 0) {
        ptr[i] = NA_REAL;
      } else {
        ptr[i] = xptr[idx];
      }
    }
  } else {
    Rf_error("Type not handled: %s", Rf_type2char(TYPEOF(x_)));
  }
  
  UNPROTECT(1);
  return res_;
}










