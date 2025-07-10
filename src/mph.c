
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
// Buckets
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define BUCKET_START_CAPACITY 1
typedef struct {
  uint8_t **key;   // Array: pointer to the keys (hash retrains a copy of original key)
  size_t *len;     // Array: the lengths of each key
  uint32_t *hash;  // Array: hash of the key
  size_t *value;   // Array: integer value for each key i.e. index of insertion order
  size_t nitems;   // the number of items in this bucket
  size_t capacity; // the capacity of this bucket (for triggering re-alloc)
} bucket_t;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// The hashmap is a collection of buckets
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  bucket_t *bucket;
  size_t nbuckets;
  size_t total_items;
} mph_t;



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// FNV1 which does not accept a seed argument
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
uint32_t fnv1a(uint8_t *data, size_t len) {
  uint32_t hash = 0x01000193;
  for (size_t i = 0; i < len; i++)     
    hash = ((uint32_t)*data++ ^ hash) * Prime;   
  return hash; 
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void mph_destroy(mph_t *mph) {
  
  if (mph == NULL) return;
  if (mph->bucket != NULL) {
    for (int i = 0; i < mph->nbuckets; i++) {
      free(mph->bucket[i].value);
      free(mph->bucket[i].hash);
      free(mph->bucket[i].len);
      free(mph->bucket[i].key);
    }
    free(mph->bucket);
  }
  free(mph);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Initialize a hashmap
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mph_t *mph_init(size_t nbuckets) {
  if (nbuckets < 1) {
    return NULL;
  }
  
  mph_t *mph = calloc(1, sizeof(mph_t));
  if (mph == NULL) {
    return NULL;
  }
  mph->nbuckets    = nbuckets;
  mph->total_items = 0;
  
  mph->bucket = calloc(nbuckets, sizeof(bucket_t));
  if (mph->bucket == NULL) {
    return NULL;
  }
  
  
  for (int i = 0; i < nbuckets; ++i) {
    mph->bucket[i].value = calloc(BUCKET_START_CAPACITY, sizeof(size_t));
    if (mph->bucket[i].value == NULL) {
      Rf_error("Failed to allocate value[%i]", i);
    }
    mph->bucket[i].hash = calloc(BUCKET_START_CAPACITY, sizeof(uint32_t));
    if (mph->bucket[i].hash == NULL) {
      Rf_error("Failed to allocate hash[%i]", i);
    }
    mph->bucket[i].key = calloc(BUCKET_START_CAPACITY, sizeof(uint8_t *));
    if (mph->bucket[i].key == NULL) {
      Rf_error("Failed to allocate key[%i]", i);
    }
    mph->bucket[i].len = calloc(BUCKET_START_CAPACITY, sizeof(size_t));
    if (mph->bucket[i].len == NULL) {
      Rf_error("Failed to allocate len[%i]", i);
    }
    mph->bucket[i].nitems = 0;
    mph->bucket[i].capacity = BUCKET_START_CAPACITY;
  }
  
  return mph;
}


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
  uint32_t nbuckets = (uint32_t)Rf_length(s_) * size_factor;
  if (nbuckets < 1) {
    Rf_error("Hash with zero buckets not possible");
  }
  
  if (Rf_asInteger(verbosity_) > 0) {
    Rprintf("N buckets: %i\n", nbuckets);
  }
  
  mph_t *mph = mph_init(nbuckets);
  if (mph == NULL) {
    Rf_error("mph_init_(): Couldn't initialise hashmap");
  }
 
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Bucket all the strings
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  for (int i = 0; i < Rf_length(s_); ++i) {
    const char *s = CHAR(STRING_ELT(s_, i));
    
    // Define the key
    uint8_t *key = (uint8_t *)s;
    size_t len   = (size_t)strlen(s);
    
    // Hash key, and calculate the bucket
    uint32_t h   = fnv1a(key, len);
    uint32_t idx = h % nbuckets;
    
    // Add key to the hashmap
    mph->bucket[idx].value[mph->bucket[idx].nitems] = i;
    mph->bucket[idx].hash [mph->bucket[idx].nitems] = h;
    mph->bucket[idx].len  [mph->bucket[idx].nitems] = len;
    mph->bucket[idx].key  [mph->bucket[idx].nitems] = (uint8_t *)CHAR(STRING_ELT(s_, i));
    
    // Bump the count of items in the hashmap
    mph->bucket[idx].nitems++;
    mph->total_items++;
    
    // If hashmap is out of room, then increase the capacity
    if (mph->bucket[idx].nitems >= mph->bucket[idx].capacity) {
      mph->bucket[idx].capacity *= 2;
      mph->bucket[idx].value = realloc(mph->bucket[idx].value, (size_t)mph->bucket[idx].capacity * sizeof(size_t));
      mph->bucket[idx].hash  = realloc(mph->bucket[idx].hash , (size_t)mph->bucket[idx].capacity * sizeof(uint32_t));
      mph->bucket[idx].len   = realloc(mph->bucket[idx].len  , (size_t)mph->bucket[idx].capacity * sizeof(size_t));
      mph->bucket[idx].key   = realloc(mph->bucket[idx].key  , (size_t)mph->bucket[idx].capacity * sizeof(uint8_t *));
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
      if (mph->bucket[i].nitems == 0) {
        ++zeros;
      } else if (mph->bucket[i].nitems == 1) {
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
    for (int i = 0; i < mph->nbuckets; ++i) {
      Rprintf("[%3i  %s] ", i, mph->bucket[i].nitems == 1 ? "1" : " ");
      for (int j = 0; j < mph->bucket[i].nitems; ++j) {
        Rprintf("%3i ", (int)mph->bucket[i].value[j]);
      }
      Rprintf("\n");
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
// Lookup a single key in the hashmap
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int mph_lookup(mph_t *mph, uint8_t *key, size_t len) {
  uint32_t h   = fnv1a(key, len);
  uint32_t idx = h % mph->nbuckets;
  
  for (int j = 0; j < mph->bucket[idx].nitems; ++j) {
    if (mph->bucket[idx].hash[j] == h && memcmp(key, mph->bucket[idx].key[j], mph->bucket[idx].len[j]) == 0) {
      return mph->bucket[idx].value[j];
    }
  }
  
  return -1;
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
  // If -1, then string was not found
  // otherwise: Convert from C 0-index to R's 1-index
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  for (int i = 0; i < Rf_length(s_); ++i) {
    const char *s = CHAR(STRING_ELT(s_, i));
    
    int idx = mph_lookup(mph, (uint8_t *)s, strlen(s));
    res[i] = idx < 0 ? NA_INTEGER : idx + 1;
  }
  
  UNPROTECT(1);
  return res_;
}




