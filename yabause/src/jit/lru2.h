#ifndef LRU_H
#define LRU_H

typedef struct
{
   int reg;

   int prev;
   int next;
} LRU_list;

typedef struct
{
   int size;
   LRU_list *list;
   int first;
} LRU;

LRU *	lru_new		(int);
void	lru_delete	(LRU *);
void	lru_use_reg	(LRU *, int);
int	lru_new_reg	(LRU *);
void	lru_print	(LRU *);

#endif
