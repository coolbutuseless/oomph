/*
 * Read in a list of keys, one per line.
 * Each line can be at most KEYMAXLEN-1 characters long.
 */

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "misc.h"
#include "arena.h"
#include "keys.h"

#include <assert.h>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Allocate more space for keys
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static void grow(Keys *kp) {
  size_t oldn, newn, newsz;
  
  oldn = kp->kmax - kp->keys;
  assert(oldn < ULONG_MAX / (2 * sizeof(char *)));
  newn = max(32, oldn * 2);
  newsz = sizeof(char *) * newn;
  kp->keys = xrealloc(kp->keys, newsz);
  kp->kmax = kp->keys + newn;
  kp->kptr = kp->keys + oldn;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Expects 'key' to be terminated with "\n" (read in via fgets)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static void savekey_file(Keys *kp, char *key) {
  char *cp;
  int len, maxchar, minchar;
  
  if (kp->kptr >= kp->kmax)
    grow(kp);
  
  maxchar = kp->maxchar;
  minchar = kp->minchar;
  
  for (cp = key; *cp != '\n' && *cp; ++cp) {
    maxchar = max(*cp, maxchar);
    minchar = min(*cp, minchar);
  }
  
  assert(*cp == '\n');  
  *cp = 0;
  len = cp - key;
  
  kp->maxchar = maxchar;
  kp->minchar = minchar;
  
  kp->maxlen = max(len, kp->maxlen);
  kp->minlen = min(len, kp->minlen);
  
  // MC fix: Create a new var 'cpx' instead of re-using 'cp' as
  // was done in the original code.
  void *cpx;
  memcpy(cpx = aralloc(kp->ap, len + 1), key, len + 1);
  *kp->kptr++ = cpx;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read keys from a file
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Keys *keynew_file(FILE *fp) {
  char key[KEYMAXLEN + 1];
  Keys *kp;
  
  kp          = xmalloc(sizeof * kp);
  kp->ap      = arnew();
  kp->keys    = kp->kmax = kp->kptr = 0;
  kp->maxchar = 0;
  kp->minchar = KEYMAXCHAR;
  kp->minlen  = KEYMAXLEN;
  kp->maxlen  = 0;
  
  while (fgets(key, sizeof key, fp))
    savekey_file(kp, key);
  
  return kp;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Expects 'key' to be terminated with NULL 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void savekey_array(Keys *kp, char *key) {
  int len, maxchar, minchar;
  
  if (kp->kptr >= kp->kmax)
    grow(kp);
  
  maxchar = kp->maxchar;
  minchar = kp->minchar;
  
  len = strlen(key);
  
  for (int i = 0; i < len; i++) {
    maxchar = max(key[i], maxchar);
    minchar = min(key[i], minchar);
  }
  
  kp->maxchar = maxchar;
  kp->minchar = minchar;
  
  kp->maxlen = max(len, kp->maxlen);
  kp->minlen = min(len, kp->minlen);
  
  // MC fix: Create a new var 'cpx' instead of re-using 'cp' as
  // was done in the original code.
  void *cpx;
  memcpy(cpx = aralloc(kp->ap, len + 1), key, len + 1);
  *kp->kptr++ = cpx;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read keys from a C character array
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Keys *keynew_array(char *vec[], int N) {
  Keys *kp;
  
  kp          = xmalloc(sizeof * kp);
  kp->ap      = arnew();
  kp->keys    = kp->kmax = kp->kptr = 0;
  kp->maxchar = 0;
  kp->minchar = KEYMAXCHAR;
  kp->minlen  = KEYMAXLEN;
  kp->maxlen  = 0;
  
  for (int i = 0; i < N; i++) {
    savekey_array(kp, vec[i]);
  }
  
  return kp;
}





//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Free the 'Keys' struction
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void keyfree(Keys *kp) {
  if (!kp)
    return;
  arfree(kp->ap);
  xfree(kp->keys);
  xfree(kp);
}
