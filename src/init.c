
// #define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>

extern SEXP mph_init_(SEXP x_, SEXP size_factor_, SEXP verbosity_);
extern SEXP mph_match_(SEXP x_, SEXP bucket_);

static const R_CallMethodDef CEntries[] = {
  
  {"mph_init_"  , (DL_FUNC) &mph_init_  , 3},
  {"mph_match_" , (DL_FUNC) &mph_match_ , 2},
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



