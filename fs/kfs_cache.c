#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include "trace.h"
#include "kfs_cache.h"
#include "kfs_io.h"


int kfs_cache_element_unmap( cache_element_t *ce);
void *kfs_cache_loop_thread( void *cache_p);



void *kfs_cache_loop_thread( void *cache_p){
    cache_t *cache = ( cache_t *) cache_p;
    struct timespec remaining, request;
    int rc, i, nsec = 200000;
    cache_element_t *el;
    uint32_t flags; 

    cache->ca_flags |= KFS_CACHE_ACTIVE;
    request.tv_sec = 0;
    request.tv_nsec = nsec;


    while( (cache->ca_flags & KFS_CACHE_EXIT_LOOP) == 0){

        if( cache->ca_flags & KFS_CACHE_PAUSE_LOOP){
            request.tv_nsec = 500000;
            nanosleep( &request, &remaining);
            request.tv_nsec = nsec;
            continue;
        }

        pthread_mutex_lock( &cache->ca_mutex);
        cache->ca_flags |= KFS_CACHE_ON_LOOP;
        for( i = 0; i < cache->ca_elements_capacity; i++){
            el = cache->ca_elements[i];
            flags = el->ce_flags;

            if((flags & KFS_CACHE_NODE_ACTIVE) == 0){
                continue;
            }

            rc = pthread_mutex_trylock( &el->ce_mutex);
            if( rc != 0){
                continue;
            }

            if( (flags & KFS_CACHE_NODE_DIRTY) != 0){
                rc = extent_write( cache->ca_fd, 
                                   el->ce_mem_ptr, 
                                   el->ce_block_addr, 
                                   el->ce_num_blocks);
                if( rc != 0){
                    TRACE_ERR("extent write error");
                }else{
                    el->ce_flags &= ~KFS_CACHE_NODE_DIRTY;
                }
            }
            if( (flags & KFS_CACHE_NODE_EVICT) != 0){
                free( el->ce_mem_ptr);
                cache->ca_elements_in_use--;
                el->ce_flags = 0;
                pthread_mutex_unlock( &el->ce_mutex);
                pthread_mutex_destroy( &el->ce_mutex);
            }else{
                pthread_mutex_unlock( &el->ce_mutex);
            }
        }
        cache->ca_flags &= ~KFS_CACHE_ON_LOOP;
        if( (cache->ca_flags & KFS_CACHE_EXIT_LOOP) != 0){
            cache->ca_flags |= KFS_CACHE_EVICTED;
        }
        pthread_mutex_unlock( &cache->ca_mutex);
        nanosleep( &request, &remaining);


    }while( (cache->ca_flags & KFS_CACHE_EVICTED) == 0);


    cache->ca_flags &= ~KFS_CACHE_ACTIVE;
    pthread_exit( NULL);
}


cache_element_t *kfs_cache_element_map( cache_t *cache, 
                                        uint64_t addr, 
                                        int numblocks, 
                                        uint32_t flags,
                                        void *(*func)(void *)   ){
    int i, rc;
    uint32_t fc;
    cache_element_t **cel, *el;
    

    el = NULL;

    pthread_mutex_lock( &cache->ca_mutex);

    /* search an empty cache slot */
    for( i = 0; i < cache->ca_elements_capacity; i++){
        cel = &cache->ca_elements[i];
        fc = cel[i]->ce_flags;

        if( fc & KFS_CACHE_NODE_ACTIVE){
            continue;
        }
        el = cel[i];
        break;
    }


    if( el != NULL){
        /* found a cache slot, fill in all the required data */
        el->ce_mem_ptr = malloc( numblocks * sizeof( KFS_BLOCKSIZE));
        if( el->ce_mem_ptr == NULL){
            TRACE_ERR("malloc error");
            el = NULL;
            goto exit0;
        }

        rc = extent_read( cache->ca_fd, el->ce_mem_ptr, addr, numblocks);
        if( rc != 0){
            TRACE_ERR("extent read error");
            free( el->ce_mem_ptr);
            el = NULL;
            goto exit0;
        }

        el->ce_block_addr = addr;
        el->ce_num_blocks = numblocks;
        el->ce_flags = flags | KFS_CACHE_ACTIVE;
        el->ce_u_time = time( NULL);
        el->ce_on_unmap_callback = func;


        if (pthread_mutex_init( &el->ce_mutex, NULL) != 0) { 
            TRACE_ERR("mutex init has failed");
            free( el->ce_mem_ptr);
            el = NULL;
            goto exit0;
        } 

        cache->ca_elements_in_use++;
    }else{
        TRACE_ERR("cache is full, can not map other extent");
    }
exit0:
    pthread_mutex_unlock( &cache->ca_mutex);
    
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

    if (pthread_mutex_init(&cache->ca_mutex, NULL) != 0) { 
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


int kfs_cache_destroy( cache_t *cache){
    int i, rc;
    cache_element_t *el;
    uint32_t flags;
    void *ret;

    if( (cache->ca_flags & KFS_CACHE_READY) == 0){
        TRACE_ERR("cache is not ready, abort");
        return(-1);
    }

    pthread_mutex_lock( &cache->ca_mutex);

    for( i = 0; i < cache->ca_elements_capacity; i++){
        el = cache->ca_elements[i];
        flags = el->ce_flags;

        if((flags & KFS_CACHE_NODE_ACTIVE) == 0){
            continue;
        }

        rc = kfs_cache_element_unmap( el);
        if( rc != 0){
            TRACE_ERR("Cache element not active");
        }
    }
    
    cache->ca_flags |= KFS_CACHE_EXIT_LOOP;    
    pthread_mutex_unlock( &cache->ca_mutex);

    pthread_join( cache->ca_thread, &ret);
    pthread_mutex_destroy( &cache->ca_mutex);

    free( cache->ca_elements);
    memset( (void *) cache, 0, sizeof( cache_t));
    return(0);
}


int kfs_cache_element_unmap( cache_element_t *ce){
    if(( ce->ce_flags & KFS_CACHE_NODE_ACTIVE) == 0){
        TRACE_ERR("cache element not active");
        return(-1);
    }


    pthread_mutex_lock( &ce->ce_mutex);
    ce->ce_flags |= KFS_CACHE_NODE_EVICT;
    pthread_mutex_lock( &ce->ce_mutex);
    return(0);
}


int kfs_cache_element_mark_dirty( cache_element_t *ce){
    if(( ce->ce_flags & KFS_CACHE_NODE_ACTIVE) == 0){
        TRACE_ERR("cache element not active");
        return(-1);
    }


    pthread_mutex_lock( &ce->ce_mutex);
    ce->ce_flags |= KFS_CACHE_NODE_DIRTY;
    pthread_mutex_lock( &ce->ce_mutex);
    return(0);
}

