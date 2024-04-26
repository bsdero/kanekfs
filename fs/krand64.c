#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "krand64.h"


uint64_t _kseed64 = 1;

void set_kseed64( uint64_t seed){
    _kseed64 = seed;
}

uint64_t krand64( uint64_t max){
    uint64_t seed, a, b, c, ac, seed0, seed1, seed2, rc;
   
    a = 0xbadbabe; 
    b = 0xc007c0ffee; 
    c = 0xdeadbeef; 

    ac = 0xb00;
    seed = _kseed64++;

    seed0 = seed - 0xbadd0d0;
    seed1 = seed;
    seed2 = seed + 0xc007dad;

    seed0 = ( (( seed1 * c) >> 32) +  (( seed2 * b) >> 32) + (seed0 * c) + 
               (seed1 * b) + (seed2 * a)  );
    seed1 = ( (( seed2 * c) >> 32) + ( seed1 * c) + ( seed2 * b));
    seed2 = ( seed2 * c) + ac;

    seed1 += seed2 >> 32; 
    seed0 += seed1 >> 32;
    
    seed0 &= 0xffffffff;
    seed1 &= 0xffffffff;


    rc = (seed0 << 32) | seed1;

    if( max > 0){
       rc = rc % max;
    }
    return( rc);
}





