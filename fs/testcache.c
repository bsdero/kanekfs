#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "dumphex.h"
#include "cache.h"

void *el_map( void *arg){
    TRACE("MAP: ptr=%p", arg);
    return( NULL);
}

void *el_evict( void *arg){
    char *str = (char *) CACHE_EL_DATAPTR(arg);
    TRACE("EVICT: %p, %s", arg, str);
    return( NULL);
}

void *el_flush( void *arg){
    char *str = (char *) CACHE_EL_DATAPTR(arg);
    TRACE("FLUSH: %p, %s", arg, str);

    return( NULL);
}

int main( int argc, char **argv){
    cache_t *cache;
    int rc;
    char *p;
    cache_element_t *el, *el2, *el3, *el4, *el5;
    char s1[] = "1 anita lava la tina";
    char s2[] = "2 dabale arroz a la zorra el abad";
    char s3[] = "3 Amo la paloma";
    char s4[] = "4 Luz azul";
    char s5[] = "5 No traces en ese carton";


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


    TRACE( "el=%p, eldata=%p",
            el, CACHE_EL_DATAPTR(el));
    p = ( char *) CACHE_EL_DATAPTR(el);
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

    
    TRACE( "el2=%p, el2data=%p",
            el2, CACHE_EL_DATAPTR(el2));
    strcpy( (char *) CACHE_EL_DATAPTR(el2), s2);
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


    TRACE("Ok basic test");
    el3 = cache_element_map( cache, sizeof(s3));
    if( el3 == NULL){
        TRACE_ERR("Issues in cache_alloc()");
        return( -1);
    }
    el4 = cache_element_map( cache, sizeof(s2));
    if( el4 == NULL){
        TRACE_ERR("Issues in cache_alloc()");
        return( -1);
    }

 
    strcpy( (char *) CACHE_EL_DATAPTR(el3), s3);
    strcpy( (char *) CACHE_EL_DATAPTR(el4), s4);

    cache_element_mark_dirty( el3);
    cache_element_mark_dirty( el4);

    rc = cache_element_wait_for_flags( el3, CACHE_EL_CLEAN, 10);
    if( rc != 0){
        TRACE_ERR("timeout waiting for CACHE_EL_CLEAN");
        return( -1);
    }

    rc = cache_element_wait_for_flags( el4, CACHE_EL_CLEAN, 10);
    if( rc != 0){
        TRACE_ERR("timeout waiting for CACHE_EL_CLEAN");
        return( -1);
    }


    TRACE("will pin el");
    cache_element_pin( el);

    el5 = cache_element_map( cache, sizeof(s5));
    if( el5 == NULL){
        TRACE_ERR("Issues in cache_alloc()");
        return( -1);
    }

    strcpy( (char *) CACHE_EL_DATAPTR(el5), s5);

    rc = cache_element_wait_for_flags( el5, CACHE_EL_ACTIVE, 10);
    if( rc != 0){
        TRACE_ERR("timeout waiting for CACHE_EL_ACTIVE");
        return( -1);
    }

    TRACE("will pin el5");
    cache_element_pin( el5);
    cache_element_mark_dirty( el);
    cache_element_mark_dirty( el2);
    cache_element_mark_dirty( el3);
    cache_element_mark_dirty( el4);
    cache_element_mark_dirty( el5);

    cache_sync( cache);

    cache_element_mark_dirty( el);
    cache_element_mark_dirty( el2);
    cache_element_mark_dirty( el3);
    cache_element_mark_dirty( el4);
    cache_element_mark_dirty( el5);
    rc = cache_destroy( cache);

    return( rc);
}


