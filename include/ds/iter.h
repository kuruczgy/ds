// SPDX-License-Identifier: GPL-3.0-only
#ifndef DS_ITER_H
#define DS_ITER_H
#include <stdbool.h>
#include <ds/vec.h>

struct str_slice {
	const char *d;
	int len;
};

struct str_slice str_as_slice(const struct str *str);
struct str str_new_from_slice(struct str_slice s);

struct str_gen {
	enum str_gen_kind {
		STR_GEN_KIND_SPLIT,
	} kind;
	union {
		struct {
			struct str_slice s;
			char c;
			int i;
		} split;
	};
};

bool str_gen_next(struct str_gen *g, struct str_slice *res);
struct str_gen str_gen_split(struct str_slice s, char c);

#endif
