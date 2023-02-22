/*
 * assembly.h
 *
 *  Created on: 2020. 5. 28.
 *      Author: Owner
 */

#ifndef COMPONENTS_ASSEMBLY_H_
#define COMPONENTS_ASSEMBLY_H_

#include <stdint.h>
#pragma once
#pragma GCC optimize ("O3")

typedef int64_t Word64;

static inline int MULSHIFT32(int x, int y) {
	return ((int64_t)x * y) >> 32;
}

static inline int FASTABS(int x) {
	int sign;

	sign = x >> (sizeof(int) * 8 - 1);
	x ^= sign;
	x -= sign;

	return x;
}

static inline int CLZ(int x) {
	int numZeros;

	if (!x)
		return (sizeof(int) * 8);

	numZeros = 0;
	while (!(x & 0x80000000)) {
		numZeros++;
		x <<= 1;
	}

	return numZeros;
}

static inline Word64 MADD64(Word64 sum, int x, int y) {
	sum += (int64_t)x * y;
	return sum;
}

static inline Word64 SHL64(Word64 x, int n) {
	return (x << n);
}

static inline Word64 SAR64(Word64 x, int n) {
	return (x >> n);
}

static inline short SAR64_Clip(Word64 x) {
	return SAR64(x, 26);
}

#endif /* COMPONENTS_ASSEMBLY_H_ */
