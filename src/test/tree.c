// SPDX-License-Identifier: GPL-3.0-only
#include "../core.h"
#include <ds/tree.h>

static void test_interval_overlap() {
	struct { long long int a1, a2, b1, b2; bool c; } d[] = {
		{ -10, 10, 0, 20, true },
		{ 5, 10, 0, 5, false },
		{ 0, 5, 5, 10, false },
		{ 0, 5, 4, 10, true },
	};
	for (int i = 0; i < sizeof(d) / sizeof(d[0]); ++i) {
		bool t = interval_overlap(
			(long long int[]){ d[i].a1, d[i].a2 },
			(long long int[]){ d[i].b1, d[i].b2 });
		asrt(t == d[i].c, "interval overlap");
	}
}

struct aug_node {
	int post_iter_test;
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
			struct aug_node *nc =
				container_of(c, struct aug_node, node);
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
			struct aug_node *nc =
				container_of(c, struct aug_node, node);
			aug += nc->aug;
		}
	}

	nx->aug = aug;
}

static void test_iter(struct rb_tree *T, int exp_n) {
	int prev = 0;
	int iter_n = 0;
	struct rb_iter iter = rb_iter(T, RB_ITER_ORDER_IN);
	struct rb_node *x;
	while (rb_iter_next(&iter, &x)) {
		++iter_n;
		struct aug_node *nx = container_of(x, struct aug_node, node);
		asrt(nx->key >= prev, "inorder iter");
		nx->post_iter_test = 0;
		prev = nx->key;
	}
	asrt(iter_n == exp_n, "exp_n");

	iter_n = 0;
	iter = rb_iter(T, RB_ITER_ORDER_POST);
	while (rb_iter_next(&iter, &x)) {
		++iter_n;
		struct aug_node *nx = container_of(x, struct aug_node, node);
		for (int i = 0; i < 2; ++i) {
			if (nx->node.child[i] != &T->nil) {
				struct aug_node *ni = container_of(
					nx->node.child[i], struct aug_node,
					node);
				asrt(ni->post_iter_test == 1, "post iter");
			}
		}
		nx->post_iter_test = 1;
	}
	asrt(iter_n == exp_n, "exp_n");
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
		test_iter(&T, i + 1);
	}
	for (int i = 0; i < 0x100; ++i) {
		struct rb_node *ni = &n[i].node;
		rb_delete(&T, ni);
		check_rb_tree(&T);
		test_iter(&T, 0x100 - i - 1);
	}
}

static void test_interval_query(struct rb_tree *T,
		long long int ran[static 2]) {
	struct rb_iter iter = rb_iter(T, RB_ITER_ORDER_IN);
	int n_exp = 0;
	struct rb_node *x;
	while (rb_iter_next(&iter, &x)) {
		struct interval_node *nx =
			container_of(x, struct interval_node, node);
		if (interval_overlap(ran, nx->ran)) ++n_exp;
	}

	int n = 0;
	long long int prev_lo = -1;
	struct interval_iter i_iter = interval_iter(T, ran);
	struct interval_node *nx;
	while (interval_iter_next(&i_iter, &nx)) {
		++n;
		asrt(prev_lo <= nx->lo, "interval_iter inorder");
		prev_lo = nx->lo;
	}

	asrt(n == n_exp, __func__);
}

static void test_interval_query_sweep(struct rb_tree *T,
		long long int ran[static 2]) {
	for (long long int i = ran[0]; i < ran[1]; ++i) {
		for (long long int j = i; j < ran[1]; ++j) {
			test_interval_query(T, (long long int[]){ i, j });
		}
	}
}

static void test_interval_tree() {
	struct rb_tree T;
	struct interval_node n[0x100];

	rb_tree_init(&T, &interval_ops);

	for (int i = 0; i < 0x100; ++i) {
		struct interval_node *ni = &n[i];
		ni->lo = (i * 8121 + 1) % 0x80;
		ni->max_hi = ni->hi = ni->lo + i;
		rb_insert(&T, &ni->node);

		test_interval_query(&T, (long long int[]){ 0x80, 0xC0 });
	}
	test_interval_query_sweep(&T, (long long int[]){ 0, 0x100 });
	for (int i = 0; i < 0x100; ++i) {
		struct rb_node *ni = &n[i].node;
		rb_delete(&T, ni);

		test_interval_query(&T, (long long int[]){ 0x80, 0xC0 });
	}
}

int main() {
	test_interval_overlap();
	test_rb_tree();
	test_interval_tree();
}
