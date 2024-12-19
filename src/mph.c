/*
 * notation:
 *  W = set of keys (from stdin)
 *  m = |W|
 *  n = c*m for some constant c>1
 *  d = 2 or 3
 *  G = (V, E) where |V|=n, |E|=m and |e|=d for e in E
 */

// Code formatting: astyle -c -s2 -p -xf -xh -A2 *.c


#define R_NO_REMAP


#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <math.h>

#include "mph.h"
#include <assert.h>




void calculate_g(mphash_struct *mphash) {
  int i;
  
  for (i = 0; i < mphash->n; i++) {
    mphash->g[i] = grnodevalue(mphash->gp, i);
  }
  
}




void ff(const char *key, int f[], mphash_struct *mphash, options *opts) {
  int i;
  unsigned v1, v2, v3;

  for (i = v1 = v2 = v3 = 0; *key; ++key) {
    v1 += mphash->T[0][i + * (unsigned char *)key];
    v2 += mphash->T[1][i + * (unsigned char *)key];
    v3 += mphash->T[2][i + * (unsigned char *)key];
    i += ALPHASZ;
    if (i >= opts->tsize)
      i = 0;
  }

  v1 %= mphash->n;
  v2 %= mphash->n;
  v3 %= mphash->n;

  /*
   * Reduce probability of loops - from Majewski's PERFES.
   */
  if (opts->loop) {
    if (v2 == v1 && ++v2 >= mphash->n)
      v2 = 0;
    if (v3 == v1 && ++v3 >= mphash->n)
      v3 = 0;
    if (v2 == v3) {
      if (++v3 >= mphash->n)
        v3 = 0;
      if (v3 == v1 && ++v3 >= mphash->n)
        v3 = 0;
    }
  }

  f[0] = v1;
  f[1] = v2;
  f[2] = v3;
}


void verify(mphash_struct *mphash, options *opts) {
  int i, j, h;
  int f[3];
  char **key;

  for (i = 0, key = mphash->kp->keys; i < mphash->m; ++i, ++key) {
    ff(*key, f, mphash, opts);
    for (j = h = 0; j < mphash->d; j++)
      h += grnodevalue(mphash->gp, f[j]);
    assert(h % mphash->m == i);
  }
}


void assign(mphash_struct *mphash) {
  int v, e, unsetv;
  int i, j, sum;

  for (v = 0; v < mphash->n; v++)
    grnodevalue(mphash->gp, v) = -1;

  for (i = 0; i < mphash->m; i++) {
    for (j = sum = 0, e = grstpop(mphash->gp); j < mphash->d; j++) {
      if (grnodevalue(mphash->gp, v = gredgenode(mphash->gp, e, j)) < 0)
        grnodevalue(mphash->gp, unsetv = v) = 0;
      else
        sum += grnodevalue(mphash->gp, v);
    }
    grnodevalue(mphash->gp, unsetv) = (gredgevalue(mphash->gp, e) - sum) % mphash->m;
    if (grnodevalue(mphash->gp, unsetv) < 0)
      grnodevalue(mphash->gp, unsetv) += mphash->m;
  }
}


void gentables(mphash_struct *mphash, options *opts) {
  int i, j, t;

  GetRNGstate();
  
  for (t = 0; t < mphash->d; t++)
    for (i = 0; i < opts->tsize; i += ALPHASZ)
      for (j = 0; j < ALPHASZ; j++) {
        // mphash->T[t][i + j] = random() % mphash->n;
        mphash->T[t][i + j] = (int)(unif_rand() * INT32_MAX) % mphash->n;
      }
      
  PutRNGstate();
  
}


int hasloop(int f[], mphash_struct *mphash) {
  return f[0] == f[1] || (mphash->d == 3 && (f[0] == f[2] || f[1] == f[2]));
}


int genedges(mphash_struct *mphash, options *opts) {
  int e, j, f[3];
  char **key;

  grdeledges(mphash->gp);

  for (e = 0, key = mphash->kp->keys; e < mphash->m; ++e, ++key) {
    ff(*key, f, mphash, opts);
    if (hasloop(f, mphash)) {
      // fprintf(stderr, "mph: loop\n");
      return -1;
    }
    for (j = 0; j < mphash->d; j++)
      gredgenode(mphash->gp, e, j) = f[j];
    graddedge(mphash->gp, e);
  }

  return 0;
}


void map(mphash_struct *mphash, state_struct *state, options *opts) {
  for (state->numiter = 0; ; ++state->numiter) {
    gentables(mphash, opts);
    if (genedges(mphash, opts) < 0)
      continue;
    if (!grcyclic(mphash->gp))
      break;
    // fprintf(stderr, "mph: cycle\n");
  }
}


void memalloc(mphash_struct *mphash, options *opts) {
  int t;

  for (t = 0; t < mphash->d; t++)
    mphash->T[t] = xmalloc(opts->tsize * sizeof(int));
  mphash->gp = grnew(mphash->n, mphash->m, mphash->d);
  
  mphash->g = xmalloc(mphash->n * sizeof(int));
  
}


void memfree(mphash_struct *mphash) {
  int t;

  grfree(mphash->gp);
  keyfree(mphash->kp);
  xfree(mphash->g);

  for (t = 0; t < mphash->d; t++) {
    xfree(mphash->T[t]);
  }
}



void readkeys_stdin(mphash_struct *mphash, state_struct *state, options *opts) {
  mphash->kp   = keynew_file(stdin);
  mphash->m    = keynum(mphash->kp);
  mphash->n    = ceil(mphash->c * mphash->m);
  opts->maxlen = min(mphash->kp->maxlen, opts->maxlen);
  opts->tsize  = opts->maxlen * ALPHASZ;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Core function for hash lookup.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int hash(mphash_struct *mphash, const char *key) {
  int i;
  unsigned f0, f1, f2;
  const unsigned char *kp = (const unsigned char *)key;
  
  for (i=0, f0=f1=f2=0; *kp; ++kp) {
    f0 += mphash->T[0][i + *kp];
    f1 += mphash->T[1][i + *kp];
    f2 += mphash->T[2][i + *kp];
    i += 256;
  }
  
  f0 %= mphash->n;
  f1 %= mphash->n;
  f2 %= mphash->n;
  
  int idx = ((mphash->g[f0] + mphash->g[f1] + mphash->g[f2]) % mphash->m);
  
  
  if (idx >= 0 && idx < mphash->nkeys && strcmp(key, mphash->kp->keys[idx]) == 0) {
    return idx; // return C 0-indexed values
  } else {
    return -1;
  }
}





