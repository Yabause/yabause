#ifndef JIT_HASH_H
#define JIT_HASH_H

#include "../core.h"
#include "lru2.h"

typedef struct
{
   int size;
   int * hash;
   u32 * bloc;
   int * list;
   LRU * lru;
} Hash;

Hash * hash_new(int);
void hash_delete(Hash *);
void hash_remove_addr(Hash *, u32);
int hash_add_addr(Hash *, u32, int *);
void hash_print(Hash *);

#endif
