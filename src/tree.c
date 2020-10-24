// SPDX-License-Identifier: GPL-3.0-only
#include <ds/tree.h>
#include "core.h"

/*
 * Implementation based on the description of red-black trees in section 13 of
 * the book "Introduction to Algorithms".
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

static void rb_transplant(struct rb_tree *T, struct rb_node *u, struct rb_node *v) {
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

void rb_iter_rec(struct rb_tree *T, void *env, rb_iter_f f, int order,
        struct rb_node *x) {
    for (int i = 0; i < 2; ++i) {
        if (i == order) {
            f(env, x);
        }

        struct rb_node *c = x->child[i];
        if (c == &T->nil) continue;
        rb_iter_rec(T, env, f, order, c);
    }
    if (order >= 2) {
        f(env, x);
    }
}
void rb_iter(struct rb_tree *T, void *env, rb_iter_f f) {
    if (T->root != &T->nil) {
        rb_iter_rec(T, env, f, 1, T->root);
    }
}

void rb_iter_post(struct rb_tree *T, void *env, rb_iter_f f) {
    if (T->root != &T->nil) {
        rb_iter_rec(T, env, f, 2, T->root);
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
        struct interval_node *nc = container_of(c, struct interval_node, node);
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
typedef void (*interval_f)(void *env, struct interval_node *n);
static void interval_query_recursive(struct rb_tree *T,
        const long long int ran[static 2], void *env, interval_f f,
        struct rb_node *x) {
    struct interval_node *nx = container_of(x, struct interval_node, node);
    if (interval_overlap(ran, nx->ran)) {
        f(env, nx);
    }
    for (int i = 0; i < 2; ++i) {
        struct rb_node *c = x->child[i];
        if (c == &T->nil) continue;
        struct interval_node *nc = container_of(c, struct interval_node, node);
        if (nc->max_hi > ran[0]) {
            interval_query_recursive(T, ran, env, f, c);
        }
    }
}
void interval_query(struct rb_tree *T,
        const long long int ran[static 2], void *env, interval_f f) {
    if (T->root != &T->nil) {
        interval_query_recursive(T, ran, env, f, T->root);
    }
}
