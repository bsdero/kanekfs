#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include "dumphex.h"


void dump_uint32( void *ptr, size_t size){
    int j;
    uint32_t *u32 = (uint32_t *) ptr;
    unsigned int i;
    unsigned char *p;
   
    size /= 4;
    printf("------------  ------------  ----\n");
    for( i = 0; i < size; i++){
        printf("[0x%08x]  [0x%08x]  ", i, u32[i]); 
        p = (unsigned char *) &u32[i];
        for( j = 3; j >= 0; j-- ){
            if( isprint( *(p + j))){
                printf("%c", *(p + j));
            }else{
                printf(".");        
            }
        }
        
        printf("\n");
    }
}

void dump_uint64( void *ptr, size_t size){
    int j;
    unsigned int i;
    unsigned char *p;
    uint64_t *u64 = ( uint64_t *) ptr;
   
    size /= 8;
    printf("--------------------  --------------------  --------\n");
    for( i = 0; i < size; i++){
        printf("[0x%016x]  [0x%016lx]  ", i, u64[i]); 
        p = (unsigned char *) &u64[i];
        for( j = 7; j >= 0; j-- ){
            if( isprint( *(p + j))){
                printf("%c", *(p + j));
            }else{
                printf(".");        
            }
        }
        
        printf("\n");
    }
}


int dumphex(  void *ptr, size_t size){
    unsigned int i, l, m;
    unsigned char *p = (unsigned char *) ptr;
                
    printf("            _0 _1 _2 _3 _4 _5 _6 _7  _8 _9 _A _B _C _D _E _F    01234567 89ABCDEF\n");
    printf("            -----------------------  -----------------------    -------- --------\n");
    
    m = l = 0;
    do{
        printf("0x%08x  ", l);
        for( i = 0; i < 8; i++, l++){
            if( l < size){
                printf("%02x ", p[i]);
            }else{
                printf("   ");
            }
        }
        
        printf(" ");
        for( ; i < 16; i++, l++){
            if( l < size){
                printf("%02x ", p[i]);
            }else{
                printf("   ");
            }
        }
        
        printf("   ");
        for( i = 0; i < 8; i++, m++){
            if( m < size)
                if( isprint( p[i] ))
                    printf("%c", p[i]);
                else
                    printf(".");
            else
                printf(" ");
        }
        
        printf(" ");
        for( ; i < 16; i++, m++){
            if( m < size)
                if( isprint( p[i] ))
                    printf("%c", p[i]);
                else
                    printf(".");
            else
                printf(" ");
        }
        
        p = p + 16;
        printf("\n");
        
        
    }while( l < size);
    printf("\n");
    
    return(0);
}

