
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
uint32_t fnv1a(uint8_t *data, size_t len) {
  uint32_t hash = 0x01000193;
  for (size_t i = 0; i < len; i++)     
    hash = ((uint32_t)*data++ ^ hash) * Prime;   
  return hash; 
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Buckets
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define BUCKET_START_CAPACITY 1
typedef struct {
  size_t *index;   // Array: Original index for each element in this bucket
  uint32_t *hash;  // Array: Hash of the string
  char **s;        // Array: pointer to the string
  size_t *len;     // Array: the lengths of the string
  size_t nitems;   // the number of items in this bucket
  size_t capacity; // the capacity of this bucket (for triggering re-alloc)
} bucket_t;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Unpack an external pointer to a C struct
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bucket_t * external_ptr_to_bucket(SEXP ptr_) {
  if (!Rf_inherits(ptr_, "mph")) {
    Rf_error("Expecting an 'mph' ExternalPtr");
  }
  
  bucket_t *bucket = TYPEOF(ptr_) != EXTPTRSXP ? NULL : (bucket_t *)R_ExternalPtrAddr(ptr_);
  if (bucket == NULL) {
    Rf_error("'mph' external pointer is invalid/NULL.");
  }
  
  return bucket;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Finalizer for an external pointer to an array of hash buckets
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static void bucket_extptr_finalizer(SEXP ptr_) {
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Unpack the pointer
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  bucket_t *bucket = (bucket_t *)R_ExternalPtrAddr(ptr_);
  
  if (bucket != NULL) {
    SEXP hash_info_   = R_ExternalPtrTag(ptr_);
    uint32_t nbuckets = (uint32_t)INTEGER(hash_info_)[0];
    // int nkeys     = (uint32_t)INTEGER(hash_info_)[1];
    
    for (int i = 0; i < nbuckets; ++i) {
      free(bucket[i].index);
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
  uint32_t nbuckets = (uint32_t)Rf_length(s_) * size_factor;
  if (nbuckets < 1) {
    Rf_error("Hash with zero buckets not possible");
  }
  bucket_t *bucket = calloc((size_t)nbuckets, sizeof(bucket_t));
  
  if (bucket == NULL) {
    Rf_error("Failed to allocate bucket_t array");
  }
  
  if (Rf_asInteger(verbosity_) > 0) {
    Rprintf("N buckets: %i\n", nbuckets);
  }
  
  
  for (int i = 0; i < nbuckets; ++i) {
    bucket[i].index = calloc(BUCKET_START_CAPACITY, sizeof(size_t));
    if (bucket[i].index == NULL) {
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
    bucket[i].len = calloc(BUCKET_START_CAPACITY, sizeof(size_t));
    if (bucket[i].len == NULL) {
      Rf_error("Failed to allocate len[%i]", i);
    }
    bucket[i].nitems = 0;
    bucket[i].capacity = BUCKET_START_CAPACITY;
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Bucket all the strings
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  for (int i = 0; i < Rf_length(s_); ++i) {
    const char *s = CHAR(STRING_ELT(s_, i));
    
    uint8_t *data = (uint8_t *)s;
    size_t len = (size_t)strlen(s);
    
    uint32_t h = fnv1a(data, len);
    uint32_t idx = h % nbuckets;
    
    bucket[idx].index[bucket[idx].nitems] = i;
    bucket[idx].hash [bucket[idx].nitems] = h;
    bucket[idx].len  [bucket[idx].nitems] = len;
    bucket[idx].s    [bucket[idx].nitems] = (char *)CHAR(STRING_ELT(s_, i));
    bucket[idx].nitems += 1;
    if (bucket[idx].nitems >= bucket[idx].capacity) {
      bucket[idx].capacity *= 2;
      bucket[idx].index = realloc(bucket[idx].index, (size_t)bucket[idx].capacity * sizeof(size_t));
      bucket[idx].hash  = realloc(bucket[idx].hash , (size_t)bucket[idx].capacity * sizeof(uint32_t));
      bucket[idx].len   = realloc(bucket[idx].len  , (size_t)bucket[idx].capacity * sizeof(size_t));
      bucket[idx].s     = realloc(bucket[idx].s    , (size_t)bucket[idx].capacity * sizeof(uint8_t *));
    }
  }
  
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Dump bucket stats
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (Rf_asInteger(verbosity_) > 0) {
    int zeros = 0;
    int ones  = 0;
    int mores = 0;
    for (int i = 0; i < nbuckets; ++i) {
      if (bucket[i].nitems == 0) {
        ++zeros;
      } else if (bucket[i].nitems == 1) {
        ++ones;
      } else {
        ++mores;
      }
    }  
    Rprintf("Items/Bucket: 0: %i,  1: %i, 2+: %i\n", zeros, ones, mores);
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Dump buckets
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (Rf_asInteger(verbosity_) > 1) {
    for (int i = 0; i < nbuckets; ++i) {
      Rprintf("[%3i  %s] ", i, bucket[i].nitems == 1 ? "1" : " ");
      for (int j = 0; j < bucket[i].nitems; ++j) {
        Rprintf("%3i ", (int)bucket[i].index[j]);
      }
      Rprintf("\n");
    }
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create an external pointer
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP hash_info_ = PROTECT(Rf_allocVector(INTSXP, 2));
  INTEGER(hash_info_)[0] = nbuckets;      // nbuckets
  INTEGER(hash_info_)[1] = Rf_length(s_); // nkeys
  
  SEXP ptr_ = PROTECT(R_MakeExternalPtr(bucket, hash_info_, s_));
  R_RegisterCFinalizer(ptr_, bucket_extptr_finalizer);
  Rf_setAttrib(ptr_, R_ClassSymbol, Rf_mkString("mph"));
  UNPROTECT(2);
  return ptr_;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Lookup a single string in the hashmap
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int mph_lookup(const char *s, bucket_t *bucket, int nbuckets) {
  uint32_t h = fnv1a((uint8_t *)s, strlen(s));
  uint32_t idx = h % nbuckets;
  
  for (int j = 0; j < bucket[idx].nitems; ++j) {
    if (bucket[idx].hash[j] == h && memcmp(s, bucket[idx].s[j], bucket[idx].len[j]) == 0) {
      return bucket[idx].index[j];
    }
  }
  
  return -1;
}  



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Hashmap match
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP mph_match_(SEXP s_, SEXP bucket_) {
  
  bucket_t *bucket = external_ptr_to_bucket(bucket_);
  SEXP hash_info_ = R_ExternalPtrTag(bucket_);
  uint32_t nbuckets = (uint32_t)INTEGER(hash_info_)[0];
  // int nkeys     = (uint32_t)INTEGER(hash_info_)[1];

  SEXP res_ = PROTECT(Rf_allocVector(INTSXP, Rf_length(s_)));
  int *res = INTEGER(res_);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // For each string, find its index. 
  // If -1, then string was not found
  // otherwise: Convert from C 0-index to R's 1-index
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  for (int i = 0; i < Rf_length(s_); ++i) {
    const char *s = CHAR(STRING_ELT(s_, i));
    
    int idx = mph_lookup(s, bucket, nbuckets);
    res[i] = idx < 0 ? NA_INTEGER : idx + 1;
  }
  
  UNPROTECT(1);
  return res_;
}




