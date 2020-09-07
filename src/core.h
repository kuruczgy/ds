#ifndef DS_CORE_H
#define DS_CORE_H
#include <stddef.h>

#ifdef DS_DEBUG
#include <stdio.h>
#include <stdlib.h>
static inline void asrt(bool b, const char *msg) {
	if (!b) {
		fprintf(stderr, "assert error msg: %s\n", msg);
		abort();
	}
}
#else
#define asrt(b, msg) ((void)(b), (void)(msg))
#endif

#define container_of(ptr, type, member) \
	(type *)((char *)(ptr) - offsetof(type, member))

#endif
