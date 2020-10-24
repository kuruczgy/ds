// SPDX-License-Identifier: GPL-3.0-only
#ifndef DS_TREE_H
#define DS_TREE_H
#include <stdbool.h>

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

typedef void (*rb_iter_f)(void *env, struct rb_node *x);
void rb_iter(struct rb_tree *T, void *env, rb_iter_f f);
void rb_iter_post(struct rb_tree *T, void *env, rb_iter_f f);

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
void interval_query(struct rb_tree *T, const long long int ran[static 2],
    void *env, void (*f)(void *env, struct interval_node *n));

#endif
