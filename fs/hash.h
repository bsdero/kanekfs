#ifndef _HASH_H_
#define _HASH_H_

#ifdef USER_SPACE
 #include <stdint.h>
 #include <string.h>
#else
 #include <linux/types.h>
 #include <linux/string.h>
#endif

/* Functions for encode words into 64-bit values for fast words sort and 
 * search. */


/* Function that create a "hash" of 10 chars words with low 
 * collisions. */
uint64_t hash_b79(char *s);
uint64_t xxh64(const void *input, const size_t len, const uint64_t seed);
uint32_t xxh32(const void *input, const size_t len, const uint32_t seed);


#endif
