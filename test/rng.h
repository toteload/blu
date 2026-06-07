#pragma once

#include "toteload.h"

always_inline
uint64_t rotl(const uint64_t x, int k) {
  return (x << k) | (x >> (64 - k));
}

always_inline
u64 splitmix64_next_from(u64 x) {
  u64 z = (x += 0x9e3779b97f4a7c15);
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
  z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
  return z ^ (z >> 31);
}

// Essentially a wrapper around the xoshiro256plusplus rng
typedef struct PRng {
  u64 s[4];
} PRng;

always_inline
void PRng_seed(PRng *rng, u64 seed) {
  u64 a = splitmix64_next_from(seed);
  u64 b = splitmix64_next_from(a);
  u64 c = splitmix64_next_from(b);
  u64 d = splitmix64_next_from(c);

  *rng = (PRng){
    .s = { a, b, c, d, },
  };
}

always_inline
u64 PRng_next(PRng *rng) {
	const u64 result = rotl(rng->s[0] + rng->s[3], 23) + rng->s[0];

	const u64 t = rng->s[1] << 17;

	rng->s[2] ^= rng->s[0];
	rng->s[3] ^= rng->s[1];
	rng->s[1] ^= rng->s[2];
	rng->s[0] ^= rng->s[3];

	rng->s[2] ^= t;

	rng->s[3] = rotl(rng->s[3], 45);

	return result;
}

always_inline
u32 PRng_u32(PRng *rng) {
  return Cast(u32, PRng_next(rng));
}

always_inline
i32 PRng_i32(PRng *rng) {
  return Cast(i32, PRng_u32(rng));
}

// Equivalent to 2^128 calls to next().
void PRng_jump(PRng *rng) {
  static const u64 JUMP[] = {
    0x180ec6d33cfd0aba,
    0xd5a61266f0c9392c,
    0xa9582618e03fc9aa,
    0x39abdc4529b1661c,
  };

	u64 s0 = 0;
	u64 s1 = 0;
	u64 s2 = 0;
	u64 s3 = 0;

	for(int i = 0; i < 4; i++) {
		for(int b = 0; b < 64; b++) {
			if (JUMP[i] & UINT64_C(1) << b) {
				s0 ^= rng->s[0];
				s1 ^= rng->s[1];
				s2 ^= rng->s[2];
				s3 ^= rng->s[3];
			}

			PRng_next(rng);	
		}
  }
		
	rng->s[0] = s0;
	rng->s[1] = s1;
	rng->s[2] = s2;
	rng->s[3] = s3;
}

// Equivalent to 2^192 calls to next().
void PRng_long_jump(PRng *rng) {
  static const u64 LONG_JUMP[] = {
    0x76e15d3efefdcbbf,
    0xc5004e441c522fb3,
    0x77710069854ee241,
    0x39109bb02acbe635,
  };

	u64 s0 = 0;
	u64 s1 = 0;
	u64 s2 = 0;
	u64 s3 = 0;

	for(int i = 0; i < 4; i++) {
		for(int b = 0; b < 64; b++) {
			if (LONG_JUMP[i] & UINT64_C(1) << b) {
				s0 ^= rng->s[0];
				s1 ^= rng->s[1];
				s2 ^= rng->s[2];
				s3 ^= rng->s[3];
			}

			PRng_next(rng);	
		}
  }
		
	rng->s[0] = s0;
	rng->s[1] = s1;
	rng->s[2] = s2;
	rng->s[3] = s3;
}

