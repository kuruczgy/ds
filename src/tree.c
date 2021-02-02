// SPDX-License-Identifier: GPL-3.0-only
#include <ds/tree.h>
#include "core.h"

/*
 * Implementation based on the description of red-black trees in section 13 of
 * the book "Introduction to Algorithms" (commonly just referred to as CLRS)
 */

enum color {
	RED = 0,
	BLACK = 1
};
enum side {
	LEFT = 0,
	RIGHT = 1
};

void rb_tree_init(struct rb_tree *T, struct rb_tree_ops *ops) {
	T->nil = (struct rb_node){ .color = BLACK };
	T->root = &T->nil;
	T->ops = ops;
}

static void rotate(struct rb_tree *T, struct rb_node *x, enum side side) {
	struct rb_node *y = x->child[!side];
	x->child[!side] = y->child[side];
	if (y->child[side] != &T->nil) {
		y->child[side]->p = x;
	}
	y->p = x->p;
	if (x->p == &T->nil) {
		T->root = y;
	} else {
		x->p->child[x != x->p->left] = y;
	}
	y->child[side] = x;
	x->p = y;

	if (T->ops->update) {
		T->ops->update(T, x);
		T->ops->update(T, y);
	}
}

static void rb_insert_fixup(struct rb_tree *T, struct rb_node *z) {
	while (z->p->color == RED) {
		int side = z->p != z->p->p->left;
		struct rb_node *y = z->p->p->child[!side];
		if (y->color == RED) {
			z->p->color = BLACK;
			y->color = BLACK;
			z->p->p->color = RED;
			z = z->p->p;
		} else {
			if (z == z->p->child[!side]) {
				z = z->p;
				rotate(T, z, side);
			}
			z->p->color = BLACK;
			z->p->p->color = RED;
			rotate(T, z->p->p, !side);
		}
	}
	T->root->color = BLACK;
}

void rb_insert(struct rb_tree *T, struct rb_node *z) {
	struct rb_node *y = &T->nil, *x = T->root;
	while (x != &T->nil) {
		y = x;
		x = x->child[!T->ops->lt(z, x)];
	}
	z->p = y;
	if (y == &T->nil) {
		T->root = z;
	} else {
		y->child[!T->ops->lt(z, y)] = z;
	}
	z->left = z->right = &T->nil;
	z->color = RED;

	if (T->ops->update) {
		struct rb_node *n = z;
		while ((n = n->p) != &T->nil) {
			T->ops->update(T, n);
		}
	}

	rb_insert_fixup(T, z);
}

static void rb_delete_fixup(struct rb_tree *T, struct rb_node *x) {
	while (x != T->root && x->color == BLACK) {
		int side = x != x->p->left;
		struct rb_node *w = x->p->child[!side];
		if (w->color == RED) {
			w->color = BLACK;
			x->p->color = RED;
			rotate(T, x->p, side);
			w = x->p->child[!side];
		}
		if (w->left->color == BLACK && w->right->color == BLACK) {
			w->color = RED;
			x = x->p;
		} else {
			if (w->child[!side]->color == BLACK) {
				w->child[side]->color = BLACK;
				w->color = RED;
				rotate(T, w, !side);
				w = x->p->child[!side];
			}
			w->color = x->p->color;
			x->p->color = BLACK;
			w->child[!side]->color = BLACK;
			rotate(T, x->p, side);
			x = T->root;
		}
	}
	x->color = BLACK;
}

static struct rb_node *rb_tree_minimum(struct rb_tree *T, struct rb_node *x) {
	while (x->left != &T->nil) {
		x = x->left;
	}
	return x;
}

static void rb_transplant(struct rb_tree *T, struct rb_node *u,
		struct rb_node *v) {
	if (u->p == &T->nil) {
		T->root = v;
	} else {
		u->p->child[u != u->p->left] = v;
	}
	v->p = u->p;
}

void rb_delete(struct rb_tree *T, struct rb_node *z) {
	struct rb_node *y = z, *x;
	enum color y_original_color = y->color;
	if (z->left == &T->nil) {
		x = z->right;
		rb_transplant(T, z, z->right);
	} else if (z->right == &T->nil) {
		x = z->left;
		rb_transplant(T, z, z->left);
	} else {
		y = rb_tree_minimum(T, z->right);
		y_original_color = y->color;
		x = y->right;
		if (y->p == z) {
			x->p = y;
		} else {
			rb_transplant(T, y, y->right);
			y->right = z->right;
			y->right->p = y;
		}
		rb_transplant(T, z, y);
		y->left = z->left;
		y->left->p = y;
		y->color = z->color;
	}

	if (T->ops->update) {
		struct rb_node *n = x;
		while ((n = n->p) != &T->nil) {
			T->ops->update(T, n);
		}
	}

	if (y_original_color == BLACK) {
		rb_delete_fixup(T, x);
	}
}

struct rb_iter rb_iter(struct rb_tree *T, enum rb_iter_order order) {
	return (struct rb_iter){
		.T = T,
		.x = T->root,
		.prev = &T->nil,
		.order = order,
	};
}

/* CLRS Exercise 10.4-5
 * see:
 * https://github.com/gzc/CLRS/blob/master/C10-Elementary-Data-Structures/
 *   10.4.md
 * https://github.com/gzc/CLRS/blob/master/C10-Elementary-Data-Structures/
 *   exercise_code/traversal.cpp
 */
bool rb_iter_next(struct rb_iter *iter, struct rb_node **res) {
	struct rb_tree *T = iter->T;
	while (iter->x != &T->nil) {
		if (iter->prev == iter->x->p) {
			iter->prev = iter->x;
			if (iter->x->left != &T->nil) {
				iter->x = iter->x->left;
			} else if (iter->x->right != &T->nil) {
				iter->x = iter->x->right;

				if (iter->order == RB_ITER_ORDER_IN) {
					*res = iter->prev;
					return true;
				}
			} else {
				iter->x = iter->x->p;

				if (iter->order == RB_ITER_ORDER_POST) {
					*res = iter->prev;
					return true;
				}
				if (iter->order == RB_ITER_ORDER_IN) {
					*res = iter->prev;
					return true;
				}
			}
		} else if (iter->prev == iter->x->left) {
			iter->prev = iter->x;
			if (iter->x->right != &T->nil) {
				iter->x = iter->x->right;
			} else {
				iter->x = iter->x->p;

				if (iter->order == RB_ITER_ORDER_POST) {
					*res = iter->prev;
					return true;
				}
			}

			if (iter->order == RB_ITER_ORDER_IN) {
				*res = iter->prev;
				return true;
			}
		} else {
			asrt(iter->prev == iter->x->right, "");
			iter->prev = iter->x;
			iter->x = iter->x->p;

			if (iter->order == RB_ITER_ORDER_POST) {
				*res = iter->prev;
				return true;
			}
		}
	}
	return false;
}

static struct rb_node *rb_minimum(struct rb_tree *T, struct rb_node *x) {
	while (x->left != &T->nil) x = x->left;
	return x;
}
struct rb_node *rb_successor(struct rb_tree *T, struct rb_node *x) {
	if (x->right != &T->nil) {
		return rb_minimum(T, x->right);
	}
	struct rb_node *y = x->p;
	while (y != &T->nil && x == y->right) {
		x = y;
		y = y->p;
	}
	return y;
}

/* note: z does not have to be in the tree, it's only passed to lt */
static struct rb_node *rb_min_greater(struct rb_tree *T, struct rb_node *z) {
	struct rb_node *y = &T->nil, *x = T->root;
	while (x != &T->nil) {
		y = x;
		x = x->child[!T->ops->lt(z, x)];
	}

	// kinda hacky, but should work for now
	while (y != &T->nil && !T->ops->lt(z, y)) {
		y = rb_successor(T, y);
	}
	return y;
}

int rb_integer_lt(struct rb_node *a, struct rb_node *b) {
	struct rb_integer_node *na =
		container_of(a, struct rb_integer_node, node);
	struct rb_integer_node *nb =
		container_of(b, struct rb_integer_node, node);
	return na->val < nb->val;
}
struct rb_tree_ops rb_integer_ops = {
	.lt = rb_integer_lt
};
struct rb_integer_node *rb_integer_min_greater(struct rb_tree *T,
	long long int min) {
	struct rb_integer_node z = { .val = min };
	struct rb_node *y = rb_min_greater(T, &z.node);
	if (y != &T->nil) {
		return container_of(y, struct rb_integer_node, node);
	} else {
		return NULL;
	}
}

int interval_lt(struct rb_node *a, struct rb_node *b) {
	struct interval_node *na = container_of(a, struct interval_node, node);
	struct interval_node *nb = container_of(b, struct interval_node, node);
	return na->lo < nb->lo;
}
void interval_update(struct rb_tree *T, struct rb_node *x) {
	struct interval_node *nx = container_of(x, struct interval_node, node);
	nx->max_hi = nx->hi;
	for (int i = 0; i < 2; ++i) {
		struct rb_node *c = x->child[i];
		if (c == &T->nil) continue;
		struct interval_node *nc =
			container_of(c, struct interval_node, node);
		if (nx->max_hi < nc->max_hi) nx->max_hi = nc->max_hi;
	}
}
struct rb_tree_ops interval_ops = {
	.lt = interval_lt, .update = interval_update
};

bool interval_overlap(const long long int a[static 2],
		const long long int b[static 2]) {
	return a[0] < b[1] && a[1] > b[0];
}

struct interval_iter interval_iter(struct rb_tree *T,
		const long long int ran[static 2]) {
	return (struct interval_iter){
		.T = T,
		.x = T->root,
		.prev = &T->nil,
		.ran = { ran[0], ran[1] },
	};
}

bool interval_iter_next(struct interval_iter *iter,
		struct interval_node **res) {
	struct rb_tree *T = iter->T;
	while (iter->x != &T->nil) {
		struct interval_node *nx =
			container_of(iter->x, struct interval_node, node);
		if (iter->prev == iter->x->p) {
			iter->prev = iter->x;
			if (iter->x->left != &T->nil) {
				struct interval_node *nc =
					container_of(iter->x->left,
						struct interval_node, node);
				if (nc->max_hi > iter->ran[0]) {
					iter->x = iter->x->left;
					continue;
				}
			}

			if (iter->x->right != &T->nil) {
				struct interval_node *nc =
					container_of(iter->x->right,
						struct interval_node, node);
				if (nx->ran[0] < iter->ran[1]
						&& nc->max_hi > iter->ran[0]) {
					iter->x = iter->x->right;
					if (interval_overlap(iter->ran,
							nx->ran)) {
						*res = nx;
						return true;
					}
					continue;
				}

			}

			iter->x = iter->x->p;
			if (interval_overlap(iter->ran, nx->ran)) {
				*res = nx;
				return true;
			}
		} else if (iter->prev == iter->x->left) {
			iter->prev = iter->x;
			if (iter->x->right != &T->nil) {
				struct interval_node *nc =
					container_of(iter->x->right,
						struct interval_node, node);
				if (nx->ran[0] < iter->ran[1]
						&& nc->max_hi > iter->ran[0]) {
					iter->x = iter->x->right;
					if (interval_overlap(iter->ran,
							nx->ran)) {
						*res = nx;
						return true;
					}
					continue;
				}
			}

			iter->x = iter->x->p;
			if (interval_overlap(iter->ran, nx->ran)) {
				*res = nx;
				return true;
			}
		} else {
			asrt(iter->prev == iter->x->right, "");
			iter->prev = iter->x;
			iter->x = iter->x->p;
		}
	}
	return false;
}

struct interval_node *interval_min_greater(struct rb_tree *T,
	long long int min) {
	struct interval_node z = { .lo = min };
	struct rb_node *y = rb_min_greater(T, &z.node);
	if (y != &T->nil) {
		return container_of(y, struct interval_node, node);
	} else {
		return NULL;
	}
}
