// SPDX-License-Identifier: GPL-3.0-only
#include <ds/hashmap.h>
#include <stdlib.h>

int main() {
	struct hashmap m;
	hashmap_init(&m, sizeof(uint32_t));

	int n = 1000000;
	uint32_t *keys = malloc(n * sizeof(uint32_t));
	for (int i = 0; i < n; ++i) {
		uint32_t data = -i;
		keys[i] = i;
		hashmap_put_u32(&m, &keys[i], &data);
	}

	hashmap_finish(&m);
}
