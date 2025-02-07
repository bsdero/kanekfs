#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "dumphex.h"
#include "page_cache.h"

int cache_dump( cache_t *cache){
    cache_element_t *el;
    pgcache_element_t *pg_el;
    int i;
    char s[80];

    printf("CACHE DUMP:\n");
    for( i = 0; i < cache->ca_elements_capacity; i++){
        el = cache->ca_elements_ptr[i];
        pg_el = ( pgcache_element_t *) el;
        if( el == NULL){
            printf( "%d: NULL\n", i);
        }else{
            printf( "%d: %lu, dump:\n", i, el->ce_id);
            dumphex( pg_el->pe_mem_ptr, 128);
            printf("---------------------------------------\n");
        }
    }
    printf("\n");
    return(0);
}


int main( int argc, char **argv){
    pgcache_t *pgcache;
    int rc, fd;
    char *filename;
    pgcache_element_t *el, *el2;
    char s1[] = "1 anita lava la tina";
    char s2[] = "2 dabale arroz a la zorra el abad";
    char s3[] = "3 Amo la paloma";
    char s4[] = "4 Luz azul";
    char s5[] = "5 No traces en ese carton";

    if( argc != 2){
        printf("Usage: testbcache <file>\n");
        return(-1);
    }

    printf("argc=%d, argv=['%s', '%s' ]\n", argc, argv[0], argv[1]);
    filename = argv[1];
    fd = open( filename, O_RDWR );
    if( fd < 0){
        TRACE_ERR( "Could not open file '%s'\n", filename);
        return( -1);
    }
       
    pgcache = pgcache_alloc( fd, 32);
    if( pgcache == NULL){
        TRACE_ERR("Issues in pgcache_alloc()");
        close( fd);
        return( -1);
    }
        

    rc = pgcache_enable_sync( pgcache );
    if( rc != 0){
        TRACE_ERR("Issues in pgcache_enable_sync()");
        pgcache_destroy( pgcache);
        close( fd);
        return( -1);
    }
        
    cache_dump( CACHE( pgcache));


    el = pgcache_element_map_sync( pgcache, 0, 1);
    if( el == NULL){
        TRACE_ERR("Issues in pgcache_element_map_sync()");
        pgcache_destroy( pgcache);
        close( fd);
        return( -1);
    }


    strcpy( (char *) el->pe_mem_ptr, s1);
    rc = pgcache_element_sync( el);
    if( rc != 0){
        TRACE_ERR("could not sync cache_element");
        return( -1);
    }



    el2 = pgcache_element_map( pgcache, 10, 2);
    if( el == NULL){
        TRACE_ERR("Issues in pgcache_element_map()");
        close( fd);
        return( -1);
    }


    cache_dump( CACHE( pgcache));
    rc = cache_element_wait_for_flags( CACHE_EL(el2), 
                                       CACHE_EL_ACTIVE, 
                                       10);
    if( rc != 0){
        TRACE_ERR("timeout waiting for CACHE_EL_ACTIVE");
        return( -1);
    }

    strcpy( (char *) el->pe_mem_ptr, s2);
    PGCACHE_EL_MARK_DIRTY( el);

    strcpy( (char *) el2->pe_mem_ptr, s3);
    cache_element_mark_dirty( CACHE_EL(el2));


    cache_dump( CACHE( pgcache));

    rc = cache_element_wait_for_flags( CACHE_EL(el), 
                                       CACHE_EL_CLEAN, 
                                       10);
    if( rc != 0){
        TRACE_ERR("timeout waiting for CACHE_EL_CLEAN");
        return( -1);
    }
    rc = cache_element_wait_for_flags( CACHE_EL(el2), 
                                       CACHE_EL_CLEAN, 
                                       10);
    if( rc != 0){
        TRACE_ERR("timeout waiting for CACHE_EL_CLEAN");
        return( -1);
    }


    rc = cache_destroy( CACHE(pgcache));
    close( fd);

    return( rc);
}


