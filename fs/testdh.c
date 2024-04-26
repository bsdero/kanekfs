#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "dumphex.h"

char str[]="Anita lava la tina. 123456789";
uint64_t u = 0xfedcab9876543210;
uint64_t b = 0xbadd0d0;

int main(){
    dumphex( ( unsigned char *) str, 24);
    dump_uint32( (uint32_t *) str, 24);
    dump_uint64( (uint64_t *) str, 24);
}


