// SPDX-License-Identifier: GPL-3.0-only
#ifndef DS_TREE_H
#define DS_TREE_H
#include <stdbool.h>
#include <stdint.h>

/* red-black tree & interval tree */
struct rb_node {
	int color;
	struct rb_node *p;
	union {
		struct { struct rb_node *left, *right; };
		struct rb_node *child[2];
	};
};
struct rb_tree_ops;
struct rb_tree {
	struct rb_node *root;
	struct rb_node nil;
	struct rb_tree_ops *ops;
};
struct rb_tree_ops {
	int (*lt)(struct rb_node *a, struct rb_node *b); /* op: a < b */

	/*
	 * Gets called to update an augment value on x. Set to NULL if unused.
	 * The augment value must be a function of the set of x's descendants
	 * (i.e. it must not depend on their order).
	 */
	void (*update)(struct rb_tree *T, struct rb_node *x);
};
void rb_tree_init(struct rb_tree *T, struct rb_tree_ops *ops);
void rb_insert(struct rb_tree *T, struct rb_node *z);
void rb_delete(struct rb_tree *T, struct rb_node *z);

enum rb_iter_order {
	RB_ITER_ORDER_IN,
	RB_ITER_ORDER_POST,
};

struct rb_iter {
	struct rb_tree *T;
	struct rb_node *x, *prev;
	enum rb_iter_order order;
};

/* Note, that during post-order traversal, it is safe to destroy the traversed
 * nodes (as long as no pointers are modified), because the algorithm only
 * examines the pointer values of nodes after they have been traversed, and
 * does not actually dereference them. */
struct rb_iter rb_iter(struct rb_tree *T, enum rb_iter_order order);
bool rb_iter_next(struct rb_iter *iter, struct rb_node **res);
struct rb_node *rb_successor(struct rb_tree *T, struct rb_node *x);

struct rb_integer_node {
	long long int val;
	struct rb_node node;
};
extern struct rb_tree_ops rb_integer_ops;
struct rb_integer_node *rb_integer_min_greater(struct rb_tree *T,
	long long int min);

/* intervals are [lo, hi) */
struct interval_node {
	union {
		struct { long long int lo, hi; };
		long long int ran[2];
	};
	long long int max_hi;

	struct rb_node node;
};
bool interval_overlap(const long long int a[static 2],
		const long long int b[static 2]);
extern struct rb_tree_ops interval_ops;

struct interval_iter {
	struct rb_tree *T;
	struct rb_node *x, *prev;
	long long int ran[2];
};

/* This will do an inorder traversal of the interval tree, guaranteeing that
 * the intervals will appear in an increasing order based on the lower limit of
 * ther intervals. */
struct interval_iter interval_iter(struct rb_tree *T,
	const long long int ran[static 2]);
bool interval_iter_next(struct interval_iter *iter,
	struct interval_node **res);

struct interval_node *interval_min_greater(struct rb_tree *T,
	long long int min);

#endif
