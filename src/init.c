
// #define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>

extern SEXP mph_init_(SEXP vec_);
extern SEXP mph_hash_(SEXP mphash_, SEXP val_);
extern SEXP mph_subset_(SEXP x_, SEXP mphash_, SEXP val_);

static const R_CallMethodDef CEntries[] = {
  
  {"mph_init_"   , (DL_FUNC) &mph_init_   , 1},
  {"mph_hash_"   , (DL_FUNC) &mph_hash_   , 2},
  {"mph_subset_" , (DL_FUNC) &mph_subset_ , 3},
  {NULL , NULL, 0}
};


void R_init_oomph(DllInfo *info) {
  R_registerRoutines(
    info,      // DllInfo
    NULL,      // .C
    CEntries,  // .Call
    NULL,      // Fortran
    NULL       // External
  );
  R_useDynamicSymbols(info, FALSE);
}



