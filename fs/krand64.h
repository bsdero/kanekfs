#ifndef _KRAND64_H_
#define _KRAND64_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void set_kseed64( uint64_t seed);
uint64_t krand64( uint64_t max);
 
#endif


