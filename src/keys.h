#ifndef KEYS_H
#define KEYS_H

#define KEYMAXCHAR	255	/* 8-bit ASCII */

#ifndef KEYMAXLEN		/* allow override */
#define KEYMAXLEN	4096
#endif

typedef struct Keys	Keys;

struct Keys {
	char	**keys, **kmax, **kptr;
	int	minlen, maxlen;
	int	minchar, maxchar;
	Arena	*ap;
};

extern Keys	*keynew_file(FILE *);
extern Keys	*keynew_array(char *vec[], int N);
extern void savekey_array(Keys *kp, char *key);

extern void	keyfree(Keys *);
extern int	keynum(Keys *);

#define keynum(kp)	(kp->kptr - kp->keys)

#endif
