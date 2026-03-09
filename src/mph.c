


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
  
  return mph;
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Lookup a single key in the hashmap
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int32_t mph_lookup(mph_t *mph, uint8_t *key, size_t len) {
  const uint64_t h = chibihash64(key, (ptrdiff_t)len, 0xdeadbeef);
  uint32_t idx = (uint32_t)(h % mph->nbuckets);
  
  while (mph->bucket[idx].key != NULL) {
    if (mph->bucket[idx].hash == h && mph->bucket[idx].len  == len &&
        memcmp(key, mph->bucket[idx].key, mph->bucket[idx].len) == 0) {
      return mph->bucket[idx].value;
    }
    idx = (idx + 1) % mph->nbuckets;
  }
  
  return 0;
}  



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Add a key to the hashmap
// The value is implicitly the current "total_items" in the hashmap
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool mph_add(mph_t *mph, uint8_t *key, size_t len) {
  // Hash key, and calculate the bucket
  const uint64_t hash = chibihash64(key, (ptrdiff_t)len, 0xdeadbeef);
  uint32_t idx  = (uint32_t)(hash % mph->nbuckets);
  const int32_t value = (int32_t)++mph->total_items;
  
  while (mph->bucket[idx].key != NULL) {
    idx = (idx + 1) % mph->nbuckets;
  }
  
  // Add key to the hashmap
  mph->bucket[idx].value = value;
  mph->bucket[idx].hash  = hash;
  mph->bucket[idx].len   = len;
  mph->bucket[idx].key   = malloc(len);
  if (mph->bucket[idx].key == NULL) {
    return false;
  }
  memcpy(mph->bucket[idx].key, key, len);
  

  
  return true;
}

