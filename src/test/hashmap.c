// SPDX-License-Identifier: GPL-3.0-only
#include "../core.h"
#include <ds/hashmap.h>

void test_prefix_keys(uint8_t *key, int n) {
	struct hashmap m;
	hashmap_init(&m, sizeof(uint32_t));

	for (int i = 0; i < n; ++i) {
		uint32_t data = i;
		asrt(hashmap_put(&m, (struct hashmap_buffer){
			.d = key, .len = i }, &data) == MAP_OK, "1");
	}

	struct hashmap_iter iter = hashmap_iter(&m);
	uint32_t *data;
	int n_test = 0;
	while (hashmap_iter_next(&iter, (void**)&data)) {
		++n_test;
	}
	asrt(n == n_test, "0");

	for (int j = 0; j < n; ++j) {
		for (int i = j; i < n; ++i) {
			uint32_t *data;
			asrt(hashmap_get(&m, (struct hashmap_buffer){
				.d = key, .len = i },
				(void**)&data) == MAP_OK, "2");
			asrt(*data == i, "3");
		}
		asrt(hashmap_del(&m, (struct hashmap_buffer){
			.d = key, .len = j }) == MAP_OK, "4");
	}

	hashmap_finish(&m);
}

int main() {
	uint8_t key_zero[128] = { 0 };
	test_prefix_keys(key_zero, 128);

	uint8_t key_lin[128];
	for (int i = 0; i < 128; ++i) key_lin[i] = i;
	test_prefix_keys(key_lin, 128);
}
