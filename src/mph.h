


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Buckets
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define BUCKET_START_CAPACITY 1
typedef struct {
  uint8_t **key;   // Array: pointer to the keys (hash retrains a copy of original key)
  size_t *len;     // Array: the lengths of each key
  uint32_t *hash;  // Array: hash of the key
  int32_t *value;   // Array: integer value for each key i.e. index of insertion order
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



void mph_destroy(mph_t *mph);
mph_t *mph_init(size_t nbuckets); 
int32_t mph_add(mph_t *mph, uint8_t *key, size_t len);
int32_t mph_lookup(mph_t *mph, uint8_t *key, size_t len);
