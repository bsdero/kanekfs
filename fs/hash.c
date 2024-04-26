#include "hash.h"
#include <stdio.h>
#include <stdlib.h> 
#include <ctype.h> 


/* In order to encode words in a 64-bits "hash", we need to encode the ascii 
 * 256 characters (8-bit) charset into a set of lesser chars. 
 *
 * This way we can get rid of  not required control and extended characters in 
 * the ascii set. We can also use the same code for some not-relevant chars 
 * for filesystems like '/', '*' or '?' and others. 
 * This would give us enough room for a good, decent "hash". 
 *
 * Why a charset of 79 chars? This is just enough for my purposes, and if we
 * consider 79 is a prime number, we can have a pretty good, decent "hash".
 *
 * We can encode 10 char words into a 64-bit number. No collisions, all clean
 * for words with only numbers, letters and selected characters like '-', '.',
 * ' ', '/', '_', '(', ')' and others. 
 *
 */


/* limit, value, and flag. 
 * Limit is the superior limit for each characters range. 
 * If flag == 0, all the characters in the range will be = val, else 
 * every value in the range will have a different value, starting with
 * val. */
typedef struct{
    int lim, val, flag;
}hb79_r_t;


hb79_r_t hb79_r[]={
    /* lim, val , flag */ 
    { 1   , 0   ,  0 }, /* null character */
    { 0x20, 1   ,  0 }, /* all ascii control codes will have a value of 1.  */
    { 0x22, 2   ,  1 }, /* space and '!' will have value of 2 and 3 respect.*/
    { 0x28, 4   ,  0 }, /* all the range from 0x22 to 0x27 will be = 4      */
    { 0x2a, 5   ,  1 }, /* parenthesis (0x28 and 0x29) will be 5 and 6.     */
    { 0x2d, 7   ,  0 }, /* chars range from 0x2a to 0x2c will be = 7.       */
    { 0x3a, 8   ,  1 }, /* range from 0x2d to 0x39( [\-\.\/0-9] will be 8-20*/
    { 0x41, 21  ,  0 }, /* range from 0x3a to 0x41 will be = 21.            */
    { 0x5b, 22  ,  1 }, /* letters [A-Z] will have a value = [22-47]        */
    { 0x5e, 48  ,  0 }, /* range from 0x5b to 0x5d will be = 48.            */
    { 0x61, 49  ,  0 }, /* range from 0x5e to 0x60 will be = 49.            */
    { 0x7b, 50  ,  1 }, /* letters [a-z] will have a value = [50-75]        */
    { 0x7d, 76  ,  0 }, /* chars 0x7b and 0x7c will be = 76                 */
    { 0x7f, 77  ,  0 }, /* chars 0x7d and 0x7e will be = 77                 */
    { 256 , 78  ,  0 }, /* all the others will be = 78.                     */
    { 0,    0,     0}};


int ascii_2_mx79( int c){
    int cc, i, liminf, limsup=0;

    for( i = 0; hb79_r[i].lim; i++){
        liminf = limsup; 
        limsup = hb79_r[i].lim;
        if( ( c >= liminf) && ( c < limsup)){
            if( hb79_r[i].flag == 0){
                cc = hb79_r[i].val; 
            }else{
                cc = c - liminf + hb79_r[i].val;
            }
            return( cc);
        }
    }
    return( -1);
}

/* hash for 10 character words, no collisions for words with only numbers and
 * letter, very low collisions probability for words with signs, colon, 
 * semicolon, sharp, ampersand, star, and others. 
 *
 * This hash is interesting because it works well for create 64-bit numbers
 * which can be used to sort words. */
uint64_t hash_b79(char *s){
    int l, i, c, cc; 
    uint64_t h;
    l = strlen( s);
    h = 0;

    for( i = 0; i < 10; i++){
        if( i < l){
            c = s[i];
            cc = ascii_2_mx79( c);
        }else{
            cc = 0;
        } 
        h *= 79;
        h += cc;
    } 

    return( h);
}


/* took this from linux kernel. I know, I am a pirate :( */

/*-*************************************
 * Constants
 **************************************/
static const uint32_t PRIME32_1 = 2654435761U;
static const uint32_t PRIME32_2 = 2246822519U;
static const uint32_t PRIME32_3 = 3266489917U;
static const uint32_t PRIME32_4 =  668265263U;
static const uint32_t PRIME32_5 =  374761393U;

static const uint64_t PRIME64_1 = 11400714785074694791ULL;
static const uint64_t PRIME64_2 = 14029467366897019727ULL;
static const uint64_t PRIME64_3 =  1609587929392839161ULL;
static const uint64_t PRIME64_4 =  9650029242287828579ULL;
static const uint64_t PRIME64_5 =  2870177450012600261ULL;


#define xxh_rotl32(x, r) ((x << r) | (x >> (32 - r)))
#define xxh_rotl64(x, r) ((x << r) | (x >> (64 - r)))

uint32_t get_unaligned_le32( const void *p){
    uint32_t *i32 = ( uint32_t *) p;

    return( *i32);
}


uint64_t get_unaligned_le64( const void *p){
    uint64_t *i64 = ( uint64_t *) p;

    return( *i64);
}

static uint64_t xxh64_round(uint64_t acc, const uint64_t input){
    acc += input * PRIME64_2;
    acc = xxh_rotl64(acc, 31);
    acc *= PRIME64_1;
    return acc;
}

static uint64_t xxh64_merge_round(uint64_t acc, uint64_t val){
    val = xxh64_round(0, val);
    acc ^= val;
    acc = acc * PRIME64_1 + PRIME64_4;
    return acc;
}


/* 64-bit unique hash for a bunch of bytes */
uint64_t xxh64(const void *input, const size_t len, const uint64_t seed){
    const uint8_t *p = (const uint8_t *)input;
    const uint8_t *const b_end = p + len;
    uint64_t h64;

    if (len >= 32) {
        const uint8_t *const limit = b_end - 32;
        uint64_t v1 = seed + PRIME64_1 + PRIME64_2;
        uint64_t v2 = seed + PRIME64_2;
        uint64_t v3 = seed + 0;
        uint64_t v4 = seed - PRIME64_1;

        do {
            v1 = xxh64_round(v1, get_unaligned_le64(p));
            p += 8;
            v2 = xxh64_round(v2, get_unaligned_le64(p));
            p += 8;
            v3 = xxh64_round(v3, get_unaligned_le64(p));
            p += 8;
            v4 = xxh64_round(v4, get_unaligned_le64(p));
            p += 8;
        } while (p <= limit);

        h64 = xxh_rotl64(v1, 1) + xxh_rotl64(v2, 7) +
            xxh_rotl64(v3, 12) + xxh_rotl64(v4, 18);
        h64 = xxh64_merge_round(h64, v1);
        h64 = xxh64_merge_round(h64, v2);
        h64 = xxh64_merge_round(h64, v3);
        h64 = xxh64_merge_round(h64, v4);

    } else {
        h64  = seed + PRIME64_5;
    }

    h64 += (uint64_t)len;

    while (p + 8 <= b_end) {
        const uint64_t k1 = xxh64_round(0, get_unaligned_le64(p));

        h64 ^= k1;
        h64 = xxh_rotl64(h64, 27) * PRIME64_1 + PRIME64_4;
        p += 8;
    }

    if (p + 4 <= b_end) {
        h64 ^= (uint64_t)(get_unaligned_le32(p)) * PRIME64_1;
        h64 = xxh_rotl64(h64, 23) * PRIME64_2 + PRIME64_3;
        p += 4;
    }

    while (p < b_end) {
        h64 ^= (*p) * PRIME64_5;
        h64 = xxh_rotl64(h64, 11) * PRIME64_1;
        p++;
    }

    h64 ^= h64 >> 33;
    h64 *= PRIME64_2;
    h64 ^= h64 >> 29;
    h64 *= PRIME64_3;
    h64 ^= h64 >> 32;

    return h64;
}


static uint32_t xxh32_round(uint32_t seed, const uint32_t input){
	seed += input * PRIME32_2;
	seed = xxh_rotl32(seed, 13);
	seed *= PRIME32_1;
	return seed;
}

uint32_t xxh32(const void *input, const size_t len, const uint32_t seed)
{
	const uint8_t *p = (const uint8_t *)input;
	const uint8_t *b_end = p + len;
	uint32_t h32;

	if (len >= 16) {
		const uint8_t *const limit = b_end - 16;
		uint32_t v1 = seed + PRIME32_1 + PRIME32_2;
		uint32_t v2 = seed + PRIME32_2;
		uint32_t v3 = seed + 0;
		uint32_t v4 = seed - PRIME32_1;

		do {
			v1 = xxh32_round(v1, get_unaligned_le32(p));
			p += 4;
			v2 = xxh32_round(v2, get_unaligned_le32(p));
			p += 4;
			v3 = xxh32_round(v3, get_unaligned_le32(p));
			p += 4;
			v4 = xxh32_round(v4, get_unaligned_le32(p));
			p += 4;
		} while (p <= limit);

		h32 = xxh_rotl32(v1, 1) + xxh_rotl32(v2, 7) +
			xxh_rotl32(v3, 12) + xxh_rotl32(v4, 18);
	} else {
		h32 = seed + PRIME32_5;
	}

	h32 += (uint32_t)len;

	while (p + 4 <= b_end) {
		h32 += get_unaligned_le32(p) * PRIME32_3;
		h32 = xxh_rotl32(h32, 17) * PRIME32_4;
		p += 4;
	}

	while (p < b_end) {
		h32 += (*p) * PRIME32_5;
		h32 = xxh_rotl32(h32, 11) * PRIME32_1;
		p++;
	}

	h32 ^= h32 >> 15;
	h32 *= PRIME32_2;
	h32 ^= h32 >> 13;
	h32 *= PRIME32_3;
	h32 ^= h32 >> 16;

	return h32;
}

