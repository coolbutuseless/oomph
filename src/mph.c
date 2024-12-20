
#define R_NO_REMAP

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

const uint32_t Prime = 0x01000193; //   16777619 
const uint32_t Seed  = 0x811C9DC5; // 2166136261


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// FNV1 which does not accept a seed argument
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
uint32_t fnv1a(const char* text) {   
  uint32_t hash = 0x01000193;
  while (*text)     
    hash = ((uint32_t)*text++ ^ hash) * Prime;   
  return hash; 
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Buckets
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define BUCKET_START_CAPACITY 1
typedef struct {
  int *members;   // integer values of original indexes
  uint32_t *hash; // the hash of the string at this index
  char **s;       // a pointer to all the strings
  int *len;       // the lengths of the strings
  int nmembers;   // the number of members
  int capacity;   // the capacity of this particular bucket
} bucket_t;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Unpack an external pointer to a C struct
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bucket_t * external_ptr_to_buckets(SEXP ptr_) {
  if (!Rf_inherits(ptr_, "bucket")) {
    Rf_error("Expecting an 'bucket' ExternalPtr");
  }
  
  bucket_t *bucket = TYPEOF(ptr_) != EXTPTRSXP ? NULL : (bucket_t *)R_ExternalPtrAddr(ptr_);
  if (bucket == NULL) {
    Rf_error("bucket pointer is invalid/NULL.");
  }
  
  return bucket;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Finalizer for an 'rctx struct' object.
//
// This function will be called when portaudio stream object gets 
// garbage collected.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static void bucket_extptr_finalizer(SEXP ptr_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Unpack the pointer
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  bucket_t *bucket = (bucket_t *)R_ExternalPtrAddr(ptr_);
  
  if (bucket != NULL) {
    uint32_t nbuckets = (uint32_t)Rf_asInteger(R_ExternalPtrTag(ptr_));
    for (int i = 0; i < nbuckets; ++i) {
      free(bucket[i].members);
      free(bucket[i].hash);
      free(bucket[i].len);
      free(bucket[i].s);
    }
    free(bucket);
  }
  
  R_ClearExternalPtr(ptr_);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Initialise the hashmap
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP mph_init_(SEXP x_, SEXP size_factor_, SEXP verbosity_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Size factor
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  double size_factor = Rf_asReal(size_factor_);
  if (size_factor < 0.5 || size_factor > 100) {
    Rf_error("Bad size factor. Should be in range [0.5, 100], but got: %.1f", size_factor);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Setup the intermediate buckets
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  uint32_t nbuckets = (uint32_t)Rf_length(x_) * size_factor;
  bucket_t *bucket = calloc((size_t)nbuckets, sizeof(bucket_t));
  
  if (bucket == NULL) {
    Rf_error("Failed to allocate bucket_t array");
  }
  
  if (Rf_asInteger(verbosity_) > 0) {
    Rprintf("N buckets: %i\n", nbuckets);
  }
  
  
  for (int i = 0; i < nbuckets; ++i) {
    bucket[i].members = calloc(BUCKET_START_CAPACITY, sizeof(int));
    if (bucket[i].members == NULL) {
      Rf_error("Failed to allocate bucket[%i]", i);
    }
    bucket[i].hash = calloc(BUCKET_START_CAPACITY, sizeof(uint32_t));
    if (bucket[i].hash == NULL) {
      Rf_error("Failed to allocate hash[%i]", i);
    }
    bucket[i].s = calloc(BUCKET_START_CAPACITY, sizeof(char *));
    if (bucket[i].s == NULL) {
      Rf_error("Failed to allocate s[%i]", i);
    }
    bucket[i].len = calloc(BUCKET_START_CAPACITY, sizeof(int));
    if (bucket[i].len == NULL) {
      Rf_error("Failed to allocate len[%i]", i);
    }
    bucket[i].nmembers = 0;
    bucket[i].capacity = BUCKET_START_CAPACITY;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Bucket all the strings
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  for (int i = 0; i < Rf_length(x_); ++i) {
    uint32_t h = fnv1a(CHAR(STRING_ELT(x_, i)));
    uint32_t idx = h % nbuckets;
    bucket[idx].members[bucket[idx].nmembers] = i;
    bucket[idx].hash   [bucket[idx].nmembers] = h;
    bucket[idx].len    [bucket[idx].nmembers] = strlen(CHAR(STRING_ELT(x_, i)));
    bucket[idx].s      [bucket[idx].nmembers] = (char *)CHAR(STRING_ELT(x_, i));
    bucket[idx].nmembers += 1;
    if (bucket[idx].nmembers >= bucket[idx].capacity) {
      bucket[idx].capacity *= 2;
      bucket[idx].members = realloc(bucket[idx].members, (size_t)bucket[idx].capacity * sizeof(int));
      bucket[idx].hash    = realloc(bucket[idx].hash   , (size_t)bucket[idx].capacity * sizeof(uint32_t));
      bucket[idx].len     = realloc(bucket[idx].len    , (size_t)bucket[idx].capacity * sizeof(int));
      bucket[idx].s       = realloc(bucket[idx].s      , (size_t)bucket[idx].capacity * sizeof(char *));
    }
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Dump bucket stats
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (Rf_asInteger(verbosity_) > 0) {
    int zeros = 0;
    int ones = 0;
    int mores = 0;
    for (int i = 0; i < nbuckets; ++i) {
      if (bucket[i].nmembers == 0) {
        ++zeros;
      } else if (bucket[i].nmembers == 1) {
        ++ones;
      } else {
        ++mores;
      }
    }  
    Rprintf("0: %i,  1: %i, 2+: %i\n", zeros, ones, mores);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Dump buckets
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (Rf_asInteger(verbosity_) > 5) {
    for (int i = 0; i < nbuckets; ++i) {
      Rprintf("[%3i  %s] ", i, bucket[i].nmembers == 1 ? "1" : " ");
      for (int j = 0; j < bucket[i].nmembers; ++j) {
        Rprintf("%3i ", bucket[i].members[j]);
      }
      Rprintf("\n");
    }
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create an external pointer
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP nbuckets_ = PROTECT(Rf_ScalarInteger(nbuckets));
  SEXP ptr_ = PROTECT(R_MakeExternalPtr(bucket, nbuckets_, x_));
  R_RegisterCFinalizer(ptr_, bucket_extptr_finalizer);
  Rf_setAttrib(ptr_, R_ClassSymbol, Rf_mkString("bucket"));
  UNPROTECT(2);
  return ptr_;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Lookup a single string in the hashmap
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int mph_lookup(const char *s, bucket_t *bucket, int nbuckets) {
  uint32_t h = fnv1a(s);
  uint32_t idx = h % nbuckets;
  
  if (bucket[idx].nmembers == 0) {
    return -1;
  } 
  
  if (bucket[idx].nmembers == 1) {
    if (bucket[idx].hash[0] == h && memcmp(s, bucket[idx].s[0], bucket[idx].len[0]) == 0) {
      return bucket[idx].members[0];
    } else {
      return -1;
    }
  }
  
  for (int j = 0; j < bucket[idx].nmembers; ++j) {
    if (bucket[idx].hash[j] == h && memcmp(s, bucket[idx].s[j], bucket[idx].len[j]) == 0) {
      return bucket[idx].members[j];
    }
  }
  
  return -1;
}  



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Hashmap match
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP mph_match_(SEXP x_, SEXP bucket_) {
  
  bucket_t *bucket = external_ptr_to_buckets(bucket_);
  uint32_t nbuckets = (uint32_t)Rf_asInteger(R_ExternalPtrTag(bucket_));
  // SEXP orig_strings_ = R_ExternalPtrProtected(bucket_);
  
  SEXP res_ = PROTECT(Rf_allocVector(INTSXP, Rf_length(x_)));
  int *res = INTEGER(res_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Test against original
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  for (int i = 0; i < Rf_length(x_); ++i) {
    const char *s = CHAR(STRING_ELT(x_, i));
    
    uint32_t h = fnv1a(s);
    uint32_t idx = h % nbuckets;
    
    if (bucket[idx].nmembers == 0) {
      res[i] = NA_INTEGER;
    } else if (bucket[idx].nmembers == 1) {
      if (bucket[idx].hash[0] == h && memcmp(s, bucket[idx].s[0], bucket[idx].len[0]) == 0) {
        res[i] = bucket[idx].members[0] + 1;
      } else {
        res[i] = NA_INTEGER;
      }
    } else {
      bool found = false;
      for (int j = 0; j < bucket[idx].nmembers; ++j) {
        if (bucket[idx].hash[j] == h && memcmp(s, bucket[idx].s[j], bucket[idx].len[j]) == 0) {
          res[i] = bucket[idx].members[j] + 1;
          found = true;
          break;
        }
      }
      if (!found) res[i] = NA_INTEGER;
    }
  }
  
  UNPROTECT(1);
  return res_;
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Subset
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP mph_subset_(SEXP elems_, SEXP x_, SEXP bucket_) {
  
  bucket_t *bucket = external_ptr_to_buckets(bucket_);
  uint32_t nbuckets = (uint32_t)Rf_asInteger(R_ExternalPtrTag(bucket_));

  // if (Rf_length(x_) != mphash->nkeys) {
  //   Rf_error("Length(x) = %i does not match number of keys %i", 
  //            (int)Rf_length(x_), mphash->nkeys);
  // }
  
  
  SEXP res_ = R_NilValue;
  
  if (TYPEOF(x_) == VECSXP) {
    res_ = PROTECT(Rf_allocVector(VECSXP, Rf_length(elems_)));
    
    for (int i = 0; i < Rf_length(elems_); i++) {
      const char *s = CHAR(STRING_ELT(elems_, i));
      int idx = mph_lookup(s, bucket, nbuckets);
      if (idx < 0) {
        SET_VECTOR_ELT(res_, i, R_NilValue);
      } else {
        SET_VECTOR_ELT(res_, i, VECTOR_ELT(x_, idx));
      }
    }
    
  } else if (Rf_isInteger(x_)) {
    int *xptr = INTEGER(x_);
    res_ = PROTECT(Rf_allocVector(INTSXP, Rf_length(elems_)));
    int *ptr = INTEGER(res_);
    for (int i = 0; i < Rf_length(elems_); i++) {
      const char *s = CHAR(STRING_ELT(elems_, i));
      int idx = mph_lookup(s, bucket, nbuckets);
      if (idx < 0) {
        ptr[i] = NA_INTEGER;
      } else {
        ptr[i] = xptr[idx];
      }
    }
  } else if (Rf_isReal(x_)) {
    double *xptr = REAL(x_);
    res_ = PROTECT(Rf_allocVector(REALSXP, Rf_length(elems_)));
    double *ptr = REAL(res_);
    for (int i = 0; i < Rf_length(elems_); i++) {
      const char *s = CHAR(STRING_ELT(elems_, i));
      int idx = mph_lookup(s, bucket, nbuckets);
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

