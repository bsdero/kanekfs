#include <stdio.h>
#include <stdlib.h>
#include "dumphex.h"
#include "cache.h"

void *el_map( void *arg){
    char *str = (char *) arg;
    char *p = strdup( str);

    return( (void *) p);
}

void *el_evict( void *arg){
    free( arg); 

    return( NULL);
}

void *el_flush( void *arg){
    char *str = (char *) arg;
    printf("FLUSH: %s\n", str);

    return( NULL);
}

int main( int argc, char **argv){
    cache_t *cache;
    int rc;
    char *p;
    cache_element_t *el, *el2;
    char s1[] = "anita lava la tina";
    char s2[] = "dabale arroz a la zorra el abad";
    char s3[] = "Amo la paloma";
    char s4[] = "Luz azul";
    char s5[] = "No traces en ese carton";


    cache = cache_alloc( 4, NULL, el_map, el_evict, el_flush);
    if( cache == NULL){
        TRACE_ERR("Issues in cache_alloc()");
        return( -1);
    }
        

    rc = cache_run( cache);
    if( rc != 0){
        TRACE_ERR("Issues in cache_start_thread()");
        return( -1);
    }
        
    rc = cache_wait_for_flags( cache, CACHE_ACTIVE, 10);
    if( rc != 0){
        TRACE_ERR("timeout waiting for CACHE_ACTIVE flag");
        return( -1);
    }


    el = cache_element_map( cache, sizeof( s1));
    if( el == NULL){
        TRACE_ERR("Issues in cache_alloc()");
        return( -1);
    }

    p = ( char *) &el[1];
    strcpy( p, s1);


    rc = cache_element_wait_for_flags( el, CACHE_EL_ACTIVE, 10);
    if( rc != 0){
        TRACE_ERR("timeout waiting for CACHE_EL_ACTIVE");
        return( -1);
    }

    cache_element_mark_dirty( el);
    rc = cache_element_wait_for_flags( el, CACHE_EL_CLEAN, 10);
    if( rc != 0){
        TRACE_ERR("timeout waiting for CACHE_EL_CLEAN");
        return( -1);
    }


    el2 = cache_element_map( cache, sizeof(s2));
    if( el2 == NULL){
        TRACE_ERR("Issues in cache_alloc()");
        return( -1);
    }
    rc = cache_element_wait_for_flags( el2, CACHE_EL_ACTIVE, 10);
    if( rc != 0){
        TRACE_ERR("timeout waiting for CACHE_EL_ACTIVE");
        return( -1);
    }

    
    p = ( char *) &el2[1];
    strcpy( p, s2);
    cache_element_mark_dirty( el);
    cache_element_mark_dirty( el2);

    rc = cache_element_wait_for_flags( el, CACHE_EL_CLEAN, 10);
    if( rc != 0){
        TRACE_ERR("timeout waiting for CACHE_EL_CLEAN");
        return( -1);
    }

    rc = cache_element_wait_for_flags( el2, CACHE_EL_CLEAN, 10);
    if( rc != 0){
        TRACE_ERR("timeout waiting for CACHE_EL_CLEAN");
        return( -1);
    }


    rc = cache_destroy( cache);

    return( rc);
}


