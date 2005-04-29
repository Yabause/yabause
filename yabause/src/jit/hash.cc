#include "hash.hh"
#include <stdlib.h>
#include <string.h>

Hash * hash_new(int s) {
	Hash *h;

	h = (Hash *) malloc(sizeof(Hash));

	h->size = s;
	h->hash = (int *) malloc(sizeof(int) * s);
	h->bloc = (Uint32 *) malloc(sizeof(Uint32) * s);
	h->list = (int *) malloc(sizeof(int) * s);
	memset(h->hash, -1, sizeof(int) * s);
	memset(h->bloc, 0xFF, sizeof(Uint32) * s);
	memset(h->list, -1, sizeof(int) * s);

	h->lru = lru_new(s);
	return h;
}

void hash_delete(Hash *h) {
	lru_delete(h->lru);

	free(h->list);
	free(h->bloc);
	free(h->hash);

	free(h);
}

void hash_remove_addr(Hash *h, Uint32 addr) {
	Uint32 hash_addr = addr % h->size;
	Uint32 a;
	int i, j;

	j = -1;
	i = h->hash[hash_addr];
	if (i == -1) {
		return;
	}
	a = h->bloc[i];
	//while (a != hash_addr) {
	while (a != addr) {
		j = i;
		i = h->list[i];
		if (i == -1) {
			return;
		}
		a = h->bloc[i];
	}

	if (j == -1) {
		h->hash[hash_addr] = h->list[i];
	}
	else {
		h->list[j] = h->list[i];
	}
	h->bloc[i] = -1;
	h->list[i] = -1;
}

int hash_add_addr(Hash *h, Uint32 addr, int * ret) {
	Uint32 hash_addr = addr % h->size;
	Uint32 bloc_addr;
	int i, j;

	if (h->hash[hash_addr] == -1) {
		bloc_addr = lru_new_reg(h->lru);
		if (h->bloc[bloc_addr] != 0xFFFFFFFF) {
			hash_remove_addr(h, h->bloc[bloc_addr]);
		}
		h->hash[hash_addr] = bloc_addr;
		h->bloc[bloc_addr] = addr;
	}
	else {
		i = h->hash[hash_addr];
		if (h->bloc[i] == addr) {
			lru_use_reg(h->lru, i);
			*ret = i;
			return 1;
		}
		
		j = h->list[i];
		while (j != -1) {
			i = j;
			if (h->bloc[i] == addr) {
				lru_use_reg(h->lru, i);
				*ret = i;
				return 1;
			}
			j = h->list[i];
		}

		bloc_addr = lru_new_reg(h->lru);
		if (h->bloc[bloc_addr] != 0xFFFFFFFF) {
			if ((h->bloc[bloc_addr] % h->size) == hash_addr) {
				h->bloc[bloc_addr] = addr;
				*ret = bloc_addr;
				return 0;
			}
			else {
				hash_remove_addr(h, h->bloc[bloc_addr]);
			}
		}
		h->list[i] = bloc_addr;
		h->bloc[bloc_addr] = addr;
	}
	*ret = bloc_addr;
	return 0;
}

void hash_print(Hash * h) {
	int i;
	for(i = 0;i < h->size;i++) {
		printf("%10x", h->hash[i]);
	}
	printf("\n");
	for(i = 0;i < h->size;i++) {
		printf("%10x", h->bloc[i]);
	}
	printf("\n");
	for(i = 0;i < h->size;i++) {
		printf("%10x", h->list[i]);
	}
	printf("\n");
	lru_print(h->lru);
}
