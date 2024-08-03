#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include "trace.h"
#include "kfs_cache.h"


void *kfs_cache_loop_thread( void *cache_p){
    cache_t *cache = ( cache_t *) cache_p;
    struct timespec remaining, request;
    int i, nsec = 200000;
    cache_element_t **ce; 
    uint32_t flags; 

    cache->ca_flags |= KFS_CACHE_ACTIVE;
    request.tv_sec = 0;
    request.tv_nsec = nsec;


    do{

        if( cache->ca_flags & KFS_CACHE_PAUSE_LOOP){
            request.tv_nsec = 500000;
            nanosleep( &request, &remaining);
            request.tv_nsec = nsec;
            continue;
        }


        cache->ca_flags |= KFS_CACHE_ON_LOOP;
        //enable cache mutex here
        for( i = 0; i < cache->ca_elements_capacity; i++){
            ce = cache->ca_elements;
            flags = ce[i]->ce_flags;
            if((flags & KFS_CACHE_NODE_ACTIVE) == 0){
                continue;
            }

            if( (flags & KFS_CACHE_NODE_DIRTY) != 0){
                //write block
            }

            if( (flags & KFS_CACHE_NODE_EVICT) != 0){
                //clear element
            }
        }
        //disable cache mutex
        //
        nanosleep( &request, &remaining);


    }while( (cache->ca_flags & KFS_CACHE_EXIT_LOOP) == 0);


    cache->ca_flags &= ~KFS_CACHE_ACTIVE;
    return( NULL);
}


cache_element_t *kfs_cache_map( cache_t *cache, 
                                uint64_t addr, 
                                int numblocks, 
                                uint32_t flags,
                                uint32_t id,
                                void *(*func)(void *)   ){
    int i;
    uint32_t fc;
    cache_element_t **cel, *el;
    int found = -1;

    //enable cache mutex here
    for( i = 0; i < cache->ca_elements_capacity; i++){
        cel = &cache->ca_elements[i];
        fc = cel[i]->ce_flags;

        if( fc & KFS_CACHE_NODE_ACTIVE){
            continue;
        }
        found = i;
        break;
    }


    if( found != 0){
        el = cel[i];
        /* reserve mem, fill in, init its mutex */
    }
    //disable cache mutex here

    return( el);
}

int kfs_cache_alloc( cache_t *cache, int fd, int num_elems, int nanosec){
    void *p;
    memset( (void *) cache, 0, sizeof( cache_t));

    p = malloc( sizeof( cache_element_t) * num_elems);
    if( p == NULL){
        TRACE_ERR("Error in malloc()");
        return(-1);
    }

    memset( p, 0, sizeof( cache_element_t) * num_elems);
   
    cache->ca_elements = p;
    cache->ca_elements_capacity = num_elems;
    cache->ca_elements_in_use = 0;
    cache->ca_nanosec = nanosec;
    cache->ca_fd = fd;

    if (pthread_mutex_init(&cache->ca_lock, NULL) != 0) { 
        TRACE_ERR("mutex init has failed"); 
        return(-1); 
    } 

    cache->ca_flags = KFS_CACHE_READY;
    return(0);
}

int kfs_cache_start_thread( cache_t *cache){
    int rc;

    if( (cache->ca_flags & KFS_CACHE_READY) == 0){
        TRACE_ERR("cache is not ready, abort");
        return(-1);
    }

    rc = pthread_create( &cache->ca_thread, 
                         NULL, 
                         kfs_cache_loop_thread,
                         cache);
    if( rc != 0){
        TRACE_ERR("pthread_create() failed");
        return( -1);
    }

    return(0);
}


int kfs_cache_destroy( cache_t *cache);

int kfs_cache_unmap( cache_t *cache, cache_element_t *ce);




