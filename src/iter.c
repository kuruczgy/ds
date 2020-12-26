// SPDX-License-Identifier: GPL-3.0-only

#include <ds/iter.h>

bool str_gen_next(struct str_gen *g, struct str_slice *res) {
	switch (g->kind) {
	case STR_GEN_KIND_SPLIT: ;
		/* this is hacky AF, please refactor this sometime! */
		int *i = &g->split.i;
		int from = *i;
		if (from == g->split.s.len) {
			++(*i);
			if (from == 0 || g->split.s.d[from - 1] == g->split.c) {
				*res = (struct str_slice){
					.d = g->split.s.d + from,
					.len = 0
				};
				return true;
			}
		}
		if (from >= g->split.s.len) return false;
		while (*i < g->split.s.len) {
			++(*i);
			if (g->split.s.d[*i - 1] == g->split.c) {
				*res = (struct str_slice){
					.d = g->split.s.d + from,
					.len = *i - from - 1
				};
				return true;
			}
		}
		*res = (struct str_slice){
			.d = g->split.s.d + from,
			.len = *i - from
		};
		return true;
	}
	return false; // unreachable
}
struct str_gen str_gen_split(struct str_slice s, char c) {
	return (struct str_gen){
		.kind = STR_GEN_KIND_SPLIT,
		.split = { .s = s, .c = c, .i = 0 },
	};
}

struct str_slice str_as_slice(const struct str *str) {
	return (struct str_slice){ .d = str->v.d, .len = str->v.len };
}
struct str str_new_from_slice(struct str_slice s) {
	struct str str = str_empty;
	str_append(&str, s.d, s.len);
	return str;
}
