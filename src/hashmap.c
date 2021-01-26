#include <ds/hashmap.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const uint32_t INITIAL_SIZE = 1 << 8;

/*
 * see:
 * The C Programming Language (2nd ed.). pp. 144
 * https://docs.oracle.com/en/java/javase/11/docs/api/java.base/java/lang/
 *   String.html#hashCode()
 * https://doi.org/10.1007/3-540-55719-9_77 (Definition 5.1)
 *
 * Note, that this could be much more optimal, since here we are only operating
 * on a single byte at a time.
 */
static uint32_t hash_buffer_t(struct hashmap_buffer buf, uint32_t t) {
	uint32_t hashval = 0;
	for (size_t i = 0; i < buf.len; ++i) {
		hashval = buf.d[i] + t * hashval;
	}
	return hashval;
}

/* This function implements double hashing (not perfectly, since h1 and h2 are
 * not independent). */
static uint32_t hash_buffer(struct hashmap *m, struct hashmap_buffer buf,
		int i) {
	uint32_t k = hash_buffer_t(buf, 31);

	uint32_t h1 = k;

	/* the constant: ((sqrt(5) - 1) / 2) * 2^32 */
	uint32_t h2 = k * 2654435769;

	/* Ensure h2 is odd. */
	h2 = 1 + 2 * h2;

	return (h1 + i * h2) % m->table_size;
}

static bool eq_buffer(struct hashmap_buffer a, struct hashmap_buffer b) {
	if (a.len != b.len) return false;
	return memcmp(a.d, b.d, a.len) == 0;
}

struct element {
	struct hashmap_buffer key;
	enum {
		NIL,
		DELETED,
		IN_USE
	} state;
	uint8_t data[];
};

void hashmap_init(struct hashmap *m, size_t itemsize) {
	m->data = calloc(INITIAL_SIZE, sizeof(struct element) + itemsize);
	if (!m->data) return;

	m->table_size = INITIAL_SIZE;
	m->size = 0;
	m->itemsize = itemsize;
}

static bool hashmap_hash(struct hashmap *m, struct hashmap_buffer key,
		uint32_t *res) {
	/* If full, return immediately */
	if (m->size >= (m->table_size/2)) return false;

	for (int i = 0; i < m->table_size; i++) {
		uint32_t curr = hash_buffer(m, key, i);

		struct element *elem =
			m->data + (sizeof(struct element) + m->itemsize) * curr;

		if (elem->state != IN_USE) {
			*res = curr;
			return true;
		}
		if (elem->state == IN_USE && eq_buffer(elem->key, key)) {
			*res = curr;
			return true;
		}
	}

	// TODO: this should never happen
	return false;
}

static int hashmap_put_internal(struct hashmap *m, struct hashmap_buffer key,
	void *value, bool rehash);

/*
 * Doubles the size of the hashmap, and rehashes all the elements
 */
static int hashmap_rehash(struct hashmap *m) {
	// table_size must remain a power of 2
	int new_size = m->table_size << 1;
	void *curr = m->data;

	/* Setup the new elements */
	struct element* temp =
		calloc(new_size, sizeof(struct element) + m->itemsize);
	if (!temp) return MAP_OMEM;

	/* Update the array */
	m->data = temp;

	/* Update the size */
	uint32_t old_size = m->table_size;
	m->table_size = new_size;
	m->size = 0;

	/* Rehash the elements */
	for (int i = 0; i < old_size; i++) {
		struct element *elem =
			curr + (sizeof(struct element) + m->itemsize) * i;
		int status;

		if (elem->state != IN_USE) continue;

		status = hashmap_put_internal(m, elem->key, elem->data, false);
		if (status != MAP_OK) return status;
	}

	free(curr);

	return MAP_OK;
}

static int hashmap_put_internal(struct hashmap *m, struct hashmap_buffer key,
		void *value, bool rehash) {
	/* Find a place to put our value */
	uint32_t index;
	while (!hashmap_hash(m, key, &index)) {
		if (!rehash || hashmap_rehash(m) == MAP_OMEM) return MAP_OMEM;
	}

	struct element *elem =
		m->data + (sizeof(struct element) + m->itemsize) * index;
	/* Set the data */
	memcpy(elem->data, value, m->itemsize);
	elem->key = key;
	elem->state = IN_USE;
	m->size++;

	return MAP_OK;
}

int hashmap_put(struct hashmap *m, struct hashmap_buffer key, void *value) {
	return hashmap_put_internal(m, key, value, true);
}

int hashmap_get(struct hashmap *m, struct hashmap_buffer key, void **arg) {
	for (int i = 0; i < m->table_size; i++) {
		uint32_t curr = hash_buffer(m, key, i);

		struct element *elem =
			m->data + (sizeof(struct element) + m->itemsize) * curr;

		if (elem->state == IN_USE) {
			if (eq_buffer(elem->key, key)) {
				*arg = (void*)elem->data;
				return MAP_OK;
			}
		} else if (elem->state == NIL) {
			// we can stop the probe, as this item was not DELETED
			break;
		}
	}

	*arg = NULL;
	return MAP_MISSING;
}

struct hashmap_iter hashmap_iter(struct hashmap *m) {
	return (struct hashmap_iter){ .m = m, .i = 0 };
}

bool hashmap_iter_next(struct hashmap_iter *iter, void **res) {
	struct hashmap *m = iter->m;

	/* On empty hashmap, return immediately */
	if (hashmap_length(m) <= 0)
		return false;

	/* Linear probing */
	while (iter->i < m->table_size) {
		struct element *elem = m->data
			+ (sizeof(struct element) + m->itemsize) * iter->i;
		++iter->i;
		if (elem->state == IN_USE) {
			*res = (void*)elem->data;
			return true;
		}
	}

	return false;
}

int hashmap_del(struct hashmap *m, struct hashmap_buffer key) {
	for (int i = 0; i < m->table_size; i++) {
		uint32_t curr = hash_buffer(m, key, i);

		struct element *elem =
			m->data + (sizeof(struct element) + m->itemsize) * curr;

		if (elem->state == IN_USE) {
			if (eq_buffer(elem->key, key)) {
				/* Blank out the fields */
				elem->state = DELETED;
				elem->key.d = NULL;

				/* Reduce the size */
				m->size--;
				return MAP_OK;
			}
		} else if (elem->state == NIL) {
			// we can stop the probe, as this item was not DELETED
			break;
		}
	}

	/* Data not found */
	return MAP_MISSING;
}

void hashmap_finish(struct hashmap *m) {
	free(m->data);
}

int hashmap_length(const struct hashmap *m) {
	return m->size;
}

int hashmap_put_cstr(struct hashmap *m, const char *key, void *value) {
	return hashmap_put(m, (struct hashmap_buffer){
		.d = (const uint8_t *)key, .len = strlen(key) }, value);
}
int hashmap_get_cstr(struct hashmap *m, const char *key, void **arg) {
	return hashmap_get(m, (struct hashmap_buffer){
		.d = (const uint8_t *)key, .len = strlen(key) }, arg);
}
int hashmap_del_cstr(struct hashmap *m, const char *key) {
	return hashmap_del(m, (struct hashmap_buffer){
		.d = (const uint8_t *)key, .len = strlen(key) });
}

int hashmap_put_u32(struct hashmap *m, uint32_t *key, void *value) {
	return hashmap_put(m, (struct hashmap_buffer){
		.d = (const uint8_t *)key, .len = sizeof(uint32_t) }, value);
}
int hashmap_get_u32(struct hashmap *m, uint32_t *key, void **arg) {
	return hashmap_get(m, (struct hashmap_buffer){
		.d = (const uint8_t *)key, .len = sizeof(uint32_t) }, arg);
}
int hashmap_del_u32(struct hashmap *m, uint32_t *key) {
	return hashmap_del(m, (struct hashmap_buffer){
		.d = (const uint8_t *)key, .len = sizeof(uint32_t) });
}
