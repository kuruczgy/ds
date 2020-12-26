// SPDX-License-Identifier: GPL-3.0-only
#include "../core.h"
#include <ds/iter.h>
#include <string.h>

static void test_str_gen_split(const char *str, char c,
		const char *res[], int len) {
	struct str_gen g = str_gen_split((struct str_slice){
		.d = str, .len = strlen(str) }, c);

	int i = 0;
	struct str_slice s;
	while (str_gen_next(&g, &s)) {
		fprintf(stderr, "`%.*s`: %d\n", s.len, s.d, s.len);
		asrt(i < len, "");
		asrt(strlen(res[i]) == s.len, "");
		asrt(strncmp(res[i], s.d, s.len) == 0, "");
		++i;
	}
	asrt(i == len, "");
}

int main() {
	test_str_gen_split("a,s,d,f,g", ',',
		(const char *[]){ "a", "s", "d", "f", "g" }, 5);
	test_str_gen_split("a,,,f,g,", ',',
		(const char *[]){ "a", "", "", "f", "g", "" }, 6);
	test_str_gen_split("", ',',
		(const char *[]){ "" }, 1);
	test_str_gen_split(",", ',',
		(const char *[]){ "", "" }, 2);
	test_str_gen_split("hello world", ' ',
		(const char *[]){ "hello", "world" }, 2);
}
