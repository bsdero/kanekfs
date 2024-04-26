#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "hash.h"


int main(){
    char s[1024];
    char *r;
    uint64_t h1, h2;
    uint32_t h3;

    do{
        memset( s, 0, 1020);
        r = gets( s);
        if( r == NULL){
            break;
        }

        h1 = hash_b79( s);
        h2 = xxh64( s, strlen(s), 1);
        h3 = xxh32( s, strlen(s), 1);

        printf("0x%016lx : 0x%016lx : 0x%08x\n", h1, h2, h3);
    }while(1);

    return(0);
}


