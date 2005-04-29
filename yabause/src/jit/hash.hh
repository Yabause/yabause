#ifndef JIT_HASH_HH
#define JIT_HASH_HH

#include "SDL.h"
#include "lru2.hh"

typedef struct {
	int size;
	int * hash;
	Uint32 * bloc;
	int * list;
	LRU * lru;
} Hash;

Hash * hash_new(int);
void hash_delete(Hash *);
void hash_remove_addr(Hash *, Uint32);
int hash_add_addr(Hash *, Uint32, int *);
void hash_print(Hash *);

#endif
