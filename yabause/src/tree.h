/*	tree.h
	Tree implementation
	Copyright (C) 2004 Lawrence Sebald	*/
	
#include <malloc.h>

#ifndef __LJSTREE_H
#define __LJSTREE_H

#ifdef __cplusplus
extern "C"	{
#endif

typedef struct __tree_node	{
	void *data;
	struct __tree_node *left;
	struct __tree_node *right;
} tree_node_t;

typedef struct __tree	{
	tree_node_t *root;
	int (*compare)(void *, void *);
} tree_t;

tree_t create_tree(int (*cmp)(void *, void *));
void add_to_tree(tree_t *t, void *d);

void *get_lowest_data(tree_t *t);
void *get_highest_data(tree_t *t);

void remove_lowest_node(tree_t *t);
void remove_highest_node(tree_t *t);

#ifdef __cplusplus
}
#endif

#endif
