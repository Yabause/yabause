#include "lru2.h"
#include <stdio.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////////////

LRU *lru_new(int size)
{
   LRU *lru;
   int i;

   if ((lru = (LRU *) malloc(sizeof(LRU))) == NULL)
      return lru;

   lru->size = size;
   lru->list = (LRU_list *) malloc(sizeof(LRU_list) * lru->size);
	
   for(i = 0;i < lru->size;i++)
   {
      lru->list[i].reg = i;

      if (i != 0)
         lru->list[i].prev = i - 1;
      else
         lru->list[i].prev = lru->size - 1;

      if (i != (lru->size - 1))
         lru->list[i].next = i + 1;
      else
         lru->list[i].next = 0;
   }

   lru->first = 0;

   return lru;
}

//////////////////////////////////////////////////////////////////////////////

void lru_delete(LRU *lru)
{
   free(lru->list);
   free(lru);
}

//////////////////////////////////////////////////////////////////////////////

void lru_use_reg(LRU *lru, int i)
{
   if (i == lru->first)
   {
      lru->first = lru->list[lru->first].next;
      return;
   }

   if (i == lru->list[lru->first].prev)
      return;

   lru->list[lru->list[i].prev].next = lru->list[i].next;
   lru->list[lru->list[i].next].prev = lru->list[i].prev;
   lru->list[i].next = lru->first;
   lru->list[i].prev = lru->list[lru->first].prev;
   lru->list[lru->list[lru->first].prev].next = i;
   lru->list[lru->first].prev = i;
}

//////////////////////////////////////////////////////////////////////////////

int lru_new_reg(LRU *lru)
{
   int ret;

   ret = lru->first;
   lru->first = lru->list[lru->first].next;
   return ret;
}

//////////////////////////////////////////////////////////////////////////////

void lru_print(LRU *lru)
{
   int i, j;

   j = lru->first;

   for(i = 0;i < lru->size;i++)
   {
      printf("%d ", lru->list[j].reg);
      j = lru->list[j].next;
   }

   printf("\n");
}

//////////////////////////////////////////////////////////////////////////////
