


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include "mph.h"
#include "chibihash.h"



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Destroy an mph and all allocated memory
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void mph_destroy(mph_t *mph) {
  
  if (mph == NULL) return;
  if (mph->bucket != NULL) {
    for (int i = 0; i < mph->nbuckets; i++) {
      free(mph->bucket[i].value);
      free(mph->bucket[i].hash);
      free(mph->bucket[i].len);
      
      // Free each of the cached copies of keys
      for (int j = 0; j < mph->bucket[i].nitems; j++) {
        free(mph->bucket[i].key[j]);
      }
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
    mph->bucket[i].nitems   = 0;
    mph->bucket[i].capacity = BUCKET_START_CAPACITY;
    mph->bucket[i].value    = malloc(BUCKET_START_CAPACITY * sizeof(int32_t));
    mph->bucket[i].hash     = malloc(BUCKET_START_CAPACITY * sizeof(uint32_t));
    mph->bucket[i].key      = malloc(BUCKET_START_CAPACITY * sizeof(uint8_t *));
    mph->bucket[i].len      = malloc(BUCKET_START_CAPACITY * sizeof(size_t));
    
    if (mph->bucket[i].value == NULL || mph->bucket[i].hash == NULL || mph->bucket[i].key == NULL ||
        mph->bucket[i].len == NULL) {
      mph_destroy(mph);
      return NULL;
    }
  }
  
  return mph;
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Lookup a single key in the hashmap
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int32_t mph_lookup(mph_t *mph, uint8_t *key, size_t len) {
  const uint64_t h = chibihash64(key, (ptrdiff_t)len, 0xdeadbeef);
  const uint32_t idx = (uint32_t)(h % mph->nbuckets);
  
  for (int j = 0; j < mph->bucket[idx].nitems; ++j) {
    if (mph->bucket[idx].hash[j] == h && 
        mph->bucket[idx].len[j]  == len &&
        memcmp(key, mph->bucket[idx].key[j], mph->bucket[idx].len[j]) == 0) {
      return mph->bucket[idx].value[j];
    }
  }
  
  return -1;
}  



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Add a key to the hashmap
// The value is implicitly the current "total_items" in the hashmap
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int32_t mph_add(mph_t *mph, uint8_t *key, size_t len) {
  // Hash key, and calculate the bucket
  const uint64_t hash = chibihash64(key, (ptrdiff_t)len, 0xdeadbeef);
  const uint32_t idx  = (uint32_t)(hash % mph->nbuckets);
  const int32_t value = (int32_t)mph->total_items;
  
  // Add key to the hashmap
  const size_t nitems = mph->bucket[idx].nitems;
  mph->bucket[idx].value[nitems] = value;
  mph->bucket[idx].hash [nitems] = hash;
  mph->bucket[idx].len  [nitems] = len;
  mph->bucket[idx].key  [nitems] = malloc(len);
  if (mph->bucket[idx].key  [nitems] == NULL) {
    return -1;
  }
  memcpy(mph->bucket[idx].key  [nitems], key, len);
  
  // Bump the count of items in the hashmap
  mph->bucket[idx].nitems++;
  mph->total_items++;
  
  // If hashmap is out of room, then increase the capacity
  if (mph->bucket[idx].nitems >= mph->bucket[idx].capacity) {
    mph->bucket[idx].capacity *= 2;
    mph->bucket[idx].value = realloc(mph->bucket[idx].value, mph->bucket[idx].capacity * sizeof(int32_t));
    mph->bucket[idx].hash  = realloc(mph->bucket[idx].hash , mph->bucket[idx].capacity * sizeof(uint64_t));
    mph->bucket[idx].len   = realloc(mph->bucket[idx].len  , mph->bucket[idx].capacity * sizeof(size_t));
    mph->bucket[idx].key   = realloc(mph->bucket[idx].key  , mph->bucket[idx].capacity * sizeof(uint8_t *));
  }
  
  return (int)value;
}

