
#define R_NO_REMAP

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdlib.h>
#include "misc.h"

#include <assert.h>

void *xmalloc(size_t sz) {
  void *p = malloc(sz);
  assert(p || !sz);
  if (p == NULL) Rf_error("xmalloc(%zu) failed", sz);
  return p;
}

void *xrealloc(void *ptr, size_t sz) {
  void *p;
  if (ptr == NULL) {
    p = malloc(sz);
  } else {
    p = realloc(ptr, sz);
  }
  assert(p || !sz);
  if (p == NULL) Rf_error("xrealloc(%zu) failed", sz);
  return p;
}

void *xcalloc(size_t n, size_t sz) {
  void *p = calloc(n, sz);
  assert(p || !n);
  if (p == NULL) Rf_error("xcalloc(%zu, %zu) failed", n, sz);
  return p;
}

void xfree(void *ptr) {
  if (ptr)
    free(ptr);
}
