/*	tree.c
	Tree implementation
	Copyright (C) 2004 Lawrence Sebald	*/

/*	This is an implementation of a Binary Search Tree.
	These trees are aranged as follows:
	                    o
	                   / \
	                  o   o
	                 / \ / \
	                 o o o o
	      Least Value^     ^Greatest Value
	This particular implementation consists of two
	structures, a tree_t structure, which contains
	a reference to the root node, and a tree_node_t
	structure which contains references to two tree
	nodes (leaves) and a reference to the data to
	be stored.                                          */

#include "tree.h"

static void __add_helper(tree_t *t, tree_node_t *n, void *d)	{
	while(1)	{
		if(t->compare(n->data, d) > 1)	{
			if(n->right != NULL)	{
				n = n->right;
			}
			else	{
				n->right = (tree_node_t *) malloc(sizeof(tree_node_t));
				n->right->left = NULL;
				n->right->right = NULL;
				n->right->data = d;
				break;
			}
		}
		else	{
			if(n->left != NULL)	{
				n = n->left;
			}
			else	{
				n->left = (tree_node_t *) malloc(sizeof(tree_node_t));
				n->left->left = NULL;
				n->left->right = NULL;
				n->left->data = d;
				break;
			}
		}
	}
}
	

tree_t create_tree(int (*cmp)(void *, void *))	{
	tree_t newtree;
	newtree.compare = cmp;
	newtree.root = NULL;
	
	return newtree;
}

void add_to_tree(tree_t *t, void *d)	{
	if(t->root == NULL)	{	
		t->root = (tree_node_t *) malloc(sizeof(tree_node_t));
		t->root->left = NULL;
		t->root->right = NULL;
		t->root->data = d;
		return;
	}
	__add_helper(t, t->root, d);
}

void *get_highest_data(tree_t *t)	{
	tree_node_t *n = t->root;
	while(n->right)	{
		n = n->right;
	}
	return n->data;
}

void *get_lowest_data(tree_t *t)	{
	tree_node_t *n = t->root;
	while(n->left)	{
		n = n->left;
	}
	return n->data;
}

void remove_highest_node(tree_t *t)	{
	tree_node_t *n = t->root;
	
	if(t->root->right == NULL)	{
		t->root = t->root->left;
		free(n);
		return;
	}
	while(n->right->right)	{
		n = n->right;
	}
	
	free(n->right);
}

void remove_lowest_node(tree_t *t)	{
	tree_node_t *n = t->root;

	if(t->root->left == NULL)	{
		t->root = t->root->left;
		free(n);
	}
	while(n->left->left)	{
		n = n->left;
	}
	
	free(n->left);
}
