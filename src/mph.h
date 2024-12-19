

#include "misc.h"
#include "arena.h"
#include "keys.h"
#include "graph.h"

#define ALPHASZ (KEYMAXCHAR+1)

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// New options struct
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  int maxlen;
  int seed;
  int emitbinary;
  int loop;
  int tsize;
} options;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// State struct
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  int numiter;
} state_struct;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// hash struct
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  int d;
  double c;
  int n;
  int m;
  int *g;
  int *T[3];
  Graph *gp;
  Keys *kp;
  int nkeys;
} mphash_struct;

void emit(mphash_struct *mphash, state_struct *state, options *opts);
void emit_mike(mphash_struct *mphash, state_struct *state, options *opts);
void calculate_g(mphash_struct *mphash);
void verify(mphash_struct *mphash, options *opts);
void assign(mphash_struct *mphash);
void map(mphash_struct *mphash, state_struct *state, options *opts);
void memalloc(mphash_struct *mphash, options *opts);
void memfree(mphash_struct *mphash);
void readkeys_array(mphash_struct *mphash, state_struct *state, options *opts);
int hash(mphash_struct *mphash, const char *key);
