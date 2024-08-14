#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include "trace.h"
#include "kfs_page_cache.h"
#include "kfs_io.h"





void *kfs_pgcache_loop_thread( void *cache_p){
    pgcache_t *cache = ( pgcache_t *) cache_p;
    struct timespec remaining, request;
    int rc, i, nsec = 500000;
    void *arg_res; 
    pgcache_element_t *el;
    uint32_t flags; 

    TRACE("start");
    cache->ca_tid = pthread_self();
    cache->ca_flags |= KFS_PGCACHE_ACTIVE;
    request.tv_sec = 0;
    request.tv_nsec = nsec;


    do{
        if( cache->ca_flags & KFS_PGCACHE_PAUSE_LOOP){
            request.tv_nsec = 1000000;
            nanosleep( &request, &remaining);
            request.tv_nsec = nsec;
            continue;
        }
 
        pthread_mutex_lock( &cache->ca_mutex);
        cache->ca_flags |= KFS_PGCACHE_ON_LOOP;
        for( i = 0; i < cache->ca_elements_capacity; i++){
            el = &cache->ca_elements[i];
            flags = el->ce_flags;

            if((flags & KFS_PGCACHE_ND_ACTIVE) == 0){
                continue;
            }

            rc = pthread_mutex_trylock( &el->ce_mutex);
            if( rc != 0){
                continue;
            }

            if( (flags & KFS_PGCACHE_ND_DIRTY) != 0){
                el->ce_count++;
                rc = extent_write( cache->ca_fd, 
                                   el->ce_mem_ptr, 
                                   el->ce_block_addr, 
                                   el->ce_num_blocks);
                if( rc != 0){
                    TRACE_ERR("extent write error");
                }else{
                    el->ce_flags &= ~KFS_PGCACHE_ND_DIRTY;
                }
                TRACE("flushed out");
            }
            if( (flags & KFS_PGCACHE_ND_EVICT) != 0){
                if( el->ce_on_unmap_callback != NULL){
                    arg_res = (void *) &rc; 
                    arg_res = el->ce_on_unmap_callback( ( void *) el );
                    if( rc != 0){
                        TRACE_ERR("error in unmap_callback, rc=%d",
                                  *((int *)arg_res) );
                    }
                }
                free( el->ce_mem_ptr);

                cache->ca_elements_in_use--;
                el->ce_flags = 0;
                pthread_mutex_unlock( &el->ce_mutex);
                pthread_mutex_destroy( &el->ce_mutex);
            }else{
                pthread_mutex_unlock( &el->ce_mutex);
            }
        }

        cache->ca_flags &= ~KFS_PGCACHE_ON_LOOP;
        if( (cache->ca_flags & KFS_PGCACHE_EXIT_LOOP) != 0){
            cache->ca_flags |= KFS_PGCACHE_EVICTED;
        }
        pthread_mutex_unlock( &cache->ca_mutex);
        nanosleep( &request, &remaining);

    }while( (cache->ca_flags & KFS_PGCACHE_EVICTED) == 0);


    cache->ca_flags &= ~KFS_PGCACHE_ACTIVE;

    TRACE("end");
    pthread_exit( NULL);
}


pgcache_element_t *kfs_pgcache_element_map( pgcache_t *cache, 
                                        uint64_t addr, 
                                        int numblocks, 
                                        uint32_t flags,
                                        void *(*func)(void *)   ){
    int i, rc;
    pgcache_element_t *el;
    
    TRACE("start");

    el = NULL;

    pthread_mutex_lock( &cache->ca_mutex);

    TRACE("in mutex");
    /* search an empty cache slot */
    for( i = 0; i < cache->ca_elements_capacity; i++){
        el = &cache->ca_elements[i];

        if( (el->ce_flags & KFS_PGCACHE_ND_ACTIVE) != 0){
            continue;
        }

        break;
    }

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

       if (pthread_mutex_init( &el->ce_mutex, NULL) != 0) { 
            TRACE_ERR("mutex init has failed");
            free( el->ce_mem_ptr);
            el = NULL;
            goto exit0;
        } 

        el->ce_block_addr = addr;
        el->ce_num_blocks = numblocks;
        el->ce_flags = flags | KFS_PGCACHE_ND_ACTIVE;
        el->ce_on_unmap_callback = func;
        el->ce_count = 1;
 
        cache->ca_elements_in_use++;
    }else{
        TRACE_ERR("cache is full, can not map other extent");
    }
exit0:

    pthread_mutex_unlock( &cache->ca_mutex);
    
    TRACE("end");
    return( el);
}



int kfs_pgcache_alloc( pgcache_t *cache, int fd, int num_elems, int nanosec){
    void *p;
    TRACE("start");
    memset( (void *) cache, 0, sizeof( pgcache_t));

    p = malloc( sizeof( pgcache_element_t) * num_elems);
    if( p == NULL){
        TRACE_ERR("Error in malloc()");
        return(-1);
    }

    memset( p, 0, sizeof( pgcache_element_t) * num_elems);
   
    cache->ca_elements = p;
    cache->ca_elements_capacity = num_elems;
    cache->ca_elements_in_use = 0;
    cache->ca_nanosec = nanosec;
    cache->ca_fd = fd;

    if (pthread_mutex_init(&cache->ca_mutex, NULL) != 0) { 
        TRACE_ERR("mutex init has failed"); 
        return(-1); 
    } 

    cache->ca_flags = KFS_PGCACHE_READY;
    TRACE("end");
    return(0);
}



int kfs_pgcache_start_thread( pgcache_t *cache){
    int rc;

    TRACE("start");

    if( (cache->ca_flags & KFS_PGCACHE_READY) == 0){
        TRACE_ERR("cache is not ready, abort");
        return(-1);
    }

    rc = pthread_create( &cache->ca_thread, 
                         NULL, 
                         kfs_pgcache_loop_thread,
                         cache);
    if( rc != 0){
        TRACE_ERR("pthread_create() failed");
        cache->ca_thread = pthread_self();
        return( -1);
    }

    TRACE("end");
    return(0);
}


int kfs_pgcache_destroy( pgcache_t *cache){
    int i, rc;
    pgcache_element_t *el;
    void *ret;
    TRACE("start");
    if( (cache->ca_flags & KFS_PGCACHE_READY) == 0){
        TRACE_ERR("cache is not ready, abort");
        return(-1);
    }

    pthread_mutex_lock( &cache->ca_mutex);
    TRACE("in mutex");
    for( i = 0; i < cache->ca_elements_capacity; i++){
        el = &cache->ca_elements[i];

        if((el->ce_flags & KFS_PGCACHE_ND_ACTIVE) == 0){
            continue;
        }
        rc = kfs_pgcache_element_unmap( el);
        if( rc != 0){
            TRACE_ERR("Cache element not active");
        }
    }
    
    cache->ca_flags |= KFS_PGCACHE_EXIT_LOOP;    
    pthread_mutex_unlock( &cache->ca_mutex);


    if( pthread_equal( cache->ca_thread, cache->ca_tid)){
        pthread_join( cache->ca_thread, &ret);
    }
    
    pthread_mutex_destroy( &cache->ca_mutex);

    free( cache->ca_elements);
    memset( (void *) cache, 0, sizeof( pgcache_t));
    TRACE("end");

    return(0);
}


int kfs_pgcache_element_unmap( pgcache_element_t *ce){
    TRACE("start");
    if(( ce->ce_flags & KFS_PGCACHE_ND_ACTIVE) == 0){
        TRACE_ERR("cache element not active");
        return(-1);
    }


    pthread_mutex_lock( &ce->ce_mutex);
    ce->ce_flags |= KFS_PGCACHE_ND_EVICT;
    pthread_mutex_unlock( &ce->ce_mutex);
    TRACE("end");
    return(0);
}


int kfs_pgcache_element_mark_dirty( pgcache_element_t *ce){
    TRACE("start");
    if(( ce->ce_flags & KFS_PGCACHE_ND_ACTIVE) == 0){
        TRACE_ERR("cache element not active");
        return(-1);
    }


    pthread_mutex_lock( &ce->ce_mutex);
    ce->ce_flags |= KFS_PGCACHE_ND_DIRTY;
    pthread_mutex_unlock( &ce->ce_mutex);
    TRACE("end");
    return(0);
}


int kfs_pgcache_sync( pgcache_t *cache){
    int i, rc;
    pgcache_element_t *el;
    TRACE("start");
    if( (cache->ca_flags & KFS_PGCACHE_READY) == 0){
        TRACE_ERR("cache is not ready, abort");
        return(-1);
    }

    pthread_mutex_lock( &cache->ca_mutex);
    for( i = 0; i < cache->ca_elements_capacity; i++){
        el = &cache->ca_elements[i];

        if((el->ce_flags & KFS_PGCACHE_ND_ACTIVE) == 0){
            continue;
        }
        rc = kfs_pgcache_element_unmap( el);
        if( rc != 0){
            TRACE_ERR("Cache element not active");
        }
    }
    
    pthread_mutex_unlock( &cache->ca_mutex);


    TRACE("end");

    return(0);
}

