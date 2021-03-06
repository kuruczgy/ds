// SPDX-License-Identifier: GPL-3.0-only
#ifndef DS_MATRIX_H
#define DS_MATRIX_H
#include <stdbool.h>

void mat3_ident(float m[static 9]);
/* A <- B * A */
void mat3_mul_l(float A[static 9], const float B[static 9]);
void mat3_t(float m[static 9]);
void mat3_tran(float m[static 9], const float t[static 2]);
void mat3_scale(float m[static 9], const float s[static 2]);
void mat3_rot(float m[static 9], float a);

/* Project a left handed coordinate system to NDC. */
void mat3_proj(float m[static 9], const int size[static 2]);

bool aabb_contains(const float aabb[static 4], const float p[static 2]);
void aabb_intersect(float out[static 4], const float a[static 4],
	const float b[static 4]);

#endif
