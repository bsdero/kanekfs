#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "trace.h"
#include "kfs_cache.h"

int kfs_cache_loop( 

int kfs_cache_alloc( cache_t *cache, int fd, int num_elems, int nanosec){
    cache_element_t *cel;
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

    cache->ca_flags = KFS_CACHE_IS_READY;
    cache->ca_thread = 
    if( pthread_create(&cache->ca_thread, 
                               NULL, 
                               &trythis, NULL); 
    return(0);
}

int kfs_cache_run( cache_t *cache){
    if( (cache->ca_flags & KFS_CACHE_IS_READY) == 0){
        TRACE_ERR("cache is not ready, abort");
        return(-1);
    }

    
}


int kfs_cache_destroy( cache_t *cache);
cache_element_t *kfs_cache_map( cache_t *cache, uint32_t addr);
int kfs_cache_unmap( cache_t *cache, cache_element_t *cache);




