#include "core.h"
#include "tree.h"

struct aug_node {
    int key, aug;
    struct rb_node node;
};

static int check_rb_tree_len(struct rb_tree *T, struct rb_node *x) {
    if (x == &T->nil) return 0;

    /* red must have 2 black children */
    if (x->color == 0) {
        asrt(x->left->color == 1, "");
        asrt(x->right->color == 1, "");
    }

    /* check the binary search tree property */
    if (x->left != &T->nil) {
        asrt(!T->ops->lt(x, x->left), "");
    }
    if (x->right != &T->nil) {
        asrt(!T->ops->lt(x->right, x), "");
    }

    /* check the black path length recursively */
    int left = check_rb_tree_len(T, x->left) + x->left->color;
    int right = check_rb_tree_len(T, x->right) + x->right->color;
    asrt(left == right, "rb tree len property violation");
    return left;
}
static void check_aug(struct rb_tree *T, struct rb_node *x) {
    if (x == &T->nil) return;

    struct aug_node *nx = container_of(x, struct aug_node, node);
    int aug = nx->key;

    for (int i = 0; i < 2; ++i) {
        struct rb_node *c = x->child[i];
        if (c != &T->nil) {
            struct aug_node *nc = container_of(c, struct aug_node, node);
            aug += nc->aug;
        }
    }

    asrt(nx->aug == aug, "wrong aug");

    check_aug(T, x->left);
    check_aug(T, x->right);
}
static void check_rb_tree(struct rb_tree *T) {
    asrt(T->root->color == 1, "root color");
    check_rb_tree_len(T, T->root);
    check_aug(T, T->root);
}

int f_lt(struct rb_node *a, struct rb_node *b) {
    struct aug_node *na = container_of(a, struct aug_node, node);
    struct aug_node *nb = container_of(b, struct aug_node, node);
    return na->key < nb->key;
}
void f_update(struct rb_tree *T, struct rb_node *x) {
    struct aug_node *nx = container_of(x, struct aug_node, node);
    int aug = nx->key;

    for (int i = 0; i < 2; ++i) {
        struct rb_node *c = x->child[i];
        if (c != &T->nil) {
            struct aug_node *nc = container_of(c, struct aug_node, node);
            aug += nc->aug;
        }
    }

    nx->aug = aug;
}

static void test_iter(void *env, struct rb_node *x) {
    int *prev = env;
    struct aug_node *nx = container_of(x, struct aug_node, node);
    asrt(nx->key >= *prev, "inorder iter");
    *prev = nx->key;
}
static void test_rb_tree() {
    struct rb_tree T;
    struct rb_tree_ops ops = { .lt = f_lt, .update = f_update };
    struct aug_node n[0x100];

    rb_tree_init(&T, &ops);

    for (int i = 0; i < 0x100; ++i) {
        struct aug_node *ni = &n[i];
        ni->key = (i * 8121 + 1) % 0x100;
        ni->aug = ni->key;
        rb_insert(&T, &ni->node);
        check_rb_tree(&T);

        int prev = 0;
        rb_iter(&T, &prev, test_iter);
    }
    for (int i = 0; i < 0x100; ++i) {
        struct rb_node *ni = &n[i].node;
        rb_delete(&T, ni);
        check_rb_tree(&T);

        int prev = 0;
        rb_iter(&T, &prev, test_iter);
    }
}

static void interval_f(void *env, struct interval_node *x) {
    int *n = env;
    ++*n;
}
static void test_interval_tree() {
    struct rb_tree T;
    struct interval_node n[0x100];

    rb_tree_init(&T, &interval_ops);

    for (int i = 0; i < 0x100; ++i) {
        struct interval_node *ni = &n[i];
        ni->lo = (i * 8121 + 1) % 0x100;
        ni->max_hi = ni->hi = ni->lo + i;
        rb_insert(&T, &ni->node);

        long long int ran[2] = { 0x80, 0xC0 };
        int j = 0, j2 = 0;
        for (int k = 0; k <= i; ++k) {
            if (interval_overlap(ran, n[k].ran)) ++j;
        }
        interval_query(&T, ran, &j2, interval_f);
        asrt(j == j2, "bad interval query");
    }
    for (int i = 0; i < 0x100; ++i) {
        struct rb_node *ni = &n[i].node;
        rb_delete(&T, ni);

        long long int ran[2] = { 0x80, 0xC0 };
        int j = 0, j2 = 0;
        for (int k = i + 1; k < 0x100; ++k) {
            if (interval_overlap(ran, n[k].ran)) ++j;
        }
        interval_query(&T, ran, &j2, interval_f);
        asrt(j == j2, "bad interval query");
    }
}

int main() {
    test_rb_tree();
    test_interval_tree();
}
