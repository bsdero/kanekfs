#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dumphex.h"
#include "gc.h"

int main(){
    gc_list_t gcl;
    char str[] = "dabale arroz a la zorra el abad";
    char *p, *q;
    char str2[128];

    gc_list_init( &gcl);
    p = gc_malloc( &gcl, 32);

    memset( p, 'a', 32);
    q = gc_strndup( &gcl, "hola", 4);
    gc_strdup( &gcl, "anita lava la tina");

    gc_mark( q);
    sprintf( str2, "%s", "debug message");
    gc_node_set_trace( p, str2);


    printf("total=%ld\n", gc_list_total_mem( &gcl));
    gc_dump_list( &gcl);
    gc_free( p);

    gc_dump_list( &gcl);
    gc_free( q);

    gc_dump_list( &gcl);
    gc_list_destroy( &gcl);

    return 0;
}



