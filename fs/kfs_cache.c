#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include "trace.h"
#include "kfs_cache.h"
#include "kfs_io.h"




void *kfs_cache_loop_thread( void *cache_p){
    cache_t *cache = ( cache_t *) cache_p;
    struct timespec remaining, request;
    int rc, i, nsec = 900000;
    cache_element_t *el;
    uint32_t flags; 

    TRACE("start");
    cache->ca_tid = pthread_self();
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
 
        pthread_mutex_lock( &cache->ca_mutex);
        cache->ca_flags |= KFS_CACHE_ON_LOOP;
        for( i = 0; i < cache->ca_elements_capacity; i++){
            el = &cache->ca_elements[i];
            flags = el->ce_flags;

            if((flags & KFS_CACHE_NODE_ACTIVE) == 0){
                continue;
            }

            rc = pthread_mutex_trylock( &el->ce_mutex);
            if( rc != 0){
                continue;
            }

            if( (flags & KFS_CACHE_NODE_DIRTY) != 0){
                TRACE("dirty, will flush i=%d", i);
                rc = extent_write( cache->ca_fd, 
                                   el->ce_mem_ptr, 
                                   el->ce_block_addr, 
                                   el->ce_num_blocks);
                if( rc != 0){
                    TRACE_ERR("extent write error");
                }else{
                    el->ce_flags &= ~KFS_CACHE_NODE_DIRTY;
                }
                TRACE("flushed out");
            }
            if( (flags & KFS_CACHE_NODE_EVICT) != 0){
                TRACE("will evict i=%d", i);
                free( el->ce_mem_ptr);
                cache->ca_elements_in_use--;
                el->ce_flags = 0;
                pthread_mutex_unlock( &el->ce_mutex);
                pthread_mutex_destroy( &el->ce_mutex);
                TRACE("evicted");
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

    TRACE("end");
    pthread_exit( NULL);
}


cache_element_t *kfs_cache_element_map( cache_t *cache, 
                                        uint64_t addr, 
                                        int numblocks, 
                                        uint32_t flags,
                                        void *(*func)(void *)   ){
    int i, rc;
    cache_element_t *el;
    
    TRACE("start");

    el = NULL;

    pthread_mutex_lock( &cache->ca_mutex);

    TRACE("in mutex");
    /* search an empty cache slot */
    for( i = 0; i < cache->ca_elements_capacity; i++){
        el = &cache->ca_elements[i];

        if( (el->ce_flags & KFS_CACHE_NODE_ACTIVE) != 0){
            continue;
        }

        break;
    }

    TRACE("i=%d, el=[0x%x]", i, el);

    if( el != NULL){
        /* found a cache slot, fill in all the required data */
        el->ce_mem_ptr = malloc( numblocks * KFS_BLOCKSIZE);
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
        el->ce_flags = flags | KFS_CACHE_NODE_ACTIVE;
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

    TRACE("el->ce_mem_ptr=0x%x", el->ce_mem_ptr);
    pthread_mutex_unlock( &cache->ca_mutex);
    
    TRACE("end");
    return( el);
}



int kfs_cache_alloc( cache_t *cache, int fd, int num_elems, int nanosec){
    void *p;
    TRACE("start");
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
    TRACE("end");
    return(0);
}



int kfs_cache_start_thread( cache_t *cache){
    int rc;

    TRACE("start");

    if( (cache->ca_flags & KFS_CACHE_READY) == 0){
        TRACE_ERR("cache is not ready, abort");
        return(-1);
    }

    TRACE("about to run thread");
    rc = pthread_create( &cache->ca_thread, 
                         NULL, 
                         kfs_cache_loop_thread,
                         cache);
    if( rc != 0){
        TRACE_ERR("pthread_create() failed");
        cache->ca_thread = pthread_self();
        return( -1);
    }

    TRACE("end");
    return(0);
}


int kfs_cache_destroy( cache_t *cache){
    int i, rc;
    cache_element_t *el;
    void *ret;
    TRACE("start");
    if( (cache->ca_flags & KFS_CACHE_READY) == 0){
        TRACE_ERR("cache is not ready, abort");
        return(-1);
    }

    pthread_mutex_lock( &cache->ca_mutex);
    TRACE("in mutex");
    for( i = 0; i < cache->ca_elements_capacity; i++){
        el = &cache->ca_elements[i];

        if((el->ce_flags & KFS_CACHE_NODE_ACTIVE) == 0){
            continue;
        }
        TRACE("will unmap el=0x%x, i=%d, flags=0x%x", el, i, el->ce_flags);
        rc = kfs_cache_element_unmap( el);
        if( rc != 0){
            TRACE_ERR("Cache element not active");
        }
    }
    
    TRACE("about out mutex, ca_thread=%d, ca_tid=%d", cache->ca_thread, cache->ca_tid);
    cache->ca_flags |= KFS_CACHE_EXIT_LOOP;    
    pthread_mutex_unlock( &cache->ca_mutex);


    if( pthread_equal( cache->ca_thread, cache->ca_tid)){
        pthread_join( cache->ca_thread, &ret);
    }
    
    pthread_mutex_destroy( &cache->ca_mutex);

    free( cache->ca_elements);
    memset( (void *) cache, 0, sizeof( cache_t));
    TRACE("end");

    return(0);
}


int kfs_cache_element_unmap( cache_element_t *ce){
    TRACE("start");
    if(( ce->ce_flags & KFS_CACHE_NODE_ACTIVE) == 0){
        TRACE_ERR("cache element not active");
        return(-1);
    }


    pthread_mutex_lock( &ce->ce_mutex);
    ce->ce_flags |= KFS_CACHE_NODE_EVICT;
    pthread_mutex_unlock( &ce->ce_mutex);
    TRACE("end");
    return(0);
}


int kfs_cache_element_mark_dirty( cache_element_t *ce){
    TRACE("start");
    if(( ce->ce_flags & KFS_CACHE_NODE_ACTIVE) == 0){
        TRACE_ERR("cache element not active");
        return(-1);
    }


    pthread_mutex_lock( &ce->ce_mutex);
    ce->ce_flags |= KFS_CACHE_NODE_DIRTY;
    pthread_mutex_unlock( &ce->ce_mutex);
    TRACE("end");
    return(0);
}

