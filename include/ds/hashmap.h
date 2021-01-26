#ifndef DS_HASHMAP_H
#define DS_HASHMAP_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#define MAP_MISSING -3  /* No such element */
#define MAP_FULL -2 	/* Hashmap is full */
#define MAP_OMEM -1 	/* Out of Memory */
#define MAP_OK 0 	/* OK */

struct hashmap_buffer {
	const uint8_t *d;
	size_t len;
};

struct hashmap {
	void *data;
	uint32_t table_size;
	int size;
	size_t itemsize;
};

struct hashmap_iter {
	struct hashmap *m;
	uint32_t i;
};

void hashmap_init(struct hashmap *m, size_t itemsize);
struct hashmap_iter hashmap_iter(struct hashmap *m);
int hashmap_put(struct hashmap *m, struct hashmap_buffer key, void *value);
int hashmap_get(struct hashmap *m, struct hashmap_buffer key, void **arg);
int hashmap_del(struct hashmap *m, struct hashmap_buffer key);
void hashmap_finish(struct hashmap *m);
int hashmap_length(const struct hashmap *m);

bool hashmap_iter_next(struct hashmap_iter *iter, void **res);

/* convenience functions */
int hashmap_put_cstr(struct hashmap *m, const char *key, void *value);
int hashmap_get_cstr(struct hashmap *m, const char *key, void **arg);
int hashmap_del_cstr(struct hashmap *m, const char *key);

int hashmap_put_u32(struct hashmap *m, uint32_t *key, void *value);
int hashmap_get_u32(struct hashmap *m, uint32_t *key, void **arg);
int hashmap_del_u32(struct hashmap *m, uint32_t *key);

#endif
