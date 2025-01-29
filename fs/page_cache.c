#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <sys/time.h>
#include "trace.h"
#include "page_cache.h"
#include "kfs_io.h"



void *pgcache_el_evict( void *arg){
    pgcache_element_t *pgcache_el = ( pgcache_element_t *) arg;
    free( pgcache_el->pe_mem_ptr);
    return( NULL);
}

void *pgcache_el_flush( void *arg){
    pgcache_element_t *pgcache_el = ( pgcache_element_t *) arg;
    int rc = extent_write( pgcache_el->pe_el->ca_fd, 
                           el->ce_mem_ptr, 
                           el->ce_block_addr, 
                           el->ce_num_blocks);
                if( rc != 0){
                    TRACE_ERR("extent write error");
                }else{
                    el->ce_flags &= ~KFS_CACHE_ND_DIRTY;
                    el->ce_flags |= KFS_CACHE_ND_CLEAN;
                } 
    return( NULL);
}




pgcache_element_t *pgcache_element_map( pgcache_t *cache, 
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

        if( (el->ce_flags & KFS_CACHE_ND_ACTIVE) != 0){
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
        el->ce_flags = flags | KFS_CACHE_ND_ACTIVE | KFS_CACHE_ND_CLEAN;
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



pgcache_t *pgcache_alloc( int fd, int elements_capacity){
    pgcache_t *pgcache;
    int rc;

    TRACE("start");

    pgcache = malloc( sizeof( pgcache_t ));
    if( pgcache == NULL){
        TRACE_ERR("Error in malloc()"); 
        goto exit0;
    }

    memset( (void *) pgcache, 0, sizeof( pgcache_t));
    rc = cache_init( &pgcache->pc_cache, 
                     elements_capacity,
                     pg_cache_on_evict,
                     pg_cache_on_flush);
    if( rc != 0){
        TRACE_ERR("pgcache->cache init has failed"); 
        free( pgcache);
        pgcache = NULL;
    }

    pgcache->fd = fd;
exit0:
    TRACE("end");

    return( pgcache);
}



int pgcache_destroy( pgcache_t *pgcache){
    TRACE("start");
    cache_disable( &el->pc_cache);
    free( pgcache);
    TRACE("end");

    return(0);
}


int pgcache_element_unmap( pgcache_element_t *ce){
    TRACE("start");
    if(( ce->ce_flags & KFS_CACHE_ND_ACTIVE) == 0){
        TRACE_ERR("cache element not active");
        return(-1);
    }


    pthread_mutex_lock( &ce->ce_mutex);
    ce->ce_flags |= KFS_CACHE_ND_EVICT;
    pthread_mutex_unlock( &ce->ce_mutex);
    TRACE("end");
    return(0);
}


int pgcache_element_mark_dirty( pgcache_element_t *ce){
    TRACE("start");
    if(( ce->ce_flags & KFS_CACHE_ND_ACTIVE) == 0){
        TRACE_ERR("cache element not active");
        return(-1);
    }


    pthread_mutex_lock( &ce->ce_mutex);
    ce->ce_flags |= KFS_CACHE_ND_DIRTY;
    ce->ce_flags &= ~KFS_CACHE_ND_CLEAN;
    pthread_mutex_unlock( &ce->ce_mutex);
    TRACE("end");
    return(0);
}


int pgcache_sync( pgcache_t *cache){
    int i, rc;
    pgcache_element_t *el;
    TRACE("start");
    if( (cache->ca_flags & KFS_CACHE_READY) == 0){
        TRACE_ERR("cache is not ready, abort");
        return(-1);
    }

    pthread_mutex_lock( &cache->ca_mutex);
    for( i = 0; i < cache->ca_elements_capacity; i++){
        el = &cache->ca_elements[i];

        if((el->ce_flags & KFS_CACHE_ND_ACTIVE) == 0){
            continue;
        }
        rc = pgcache_element_unmap( el);
        if( rc != 0){
            TRACE_ERR("Cache element not active");
        }
    }
    
    pthread_mutex_unlock( &cache->ca_mutex);


    TRACE("end");

    return(0);
}

/* if flags == 0, it will exit when (cache->ca_flags == 0).
 * if flags != 0, it will exit when (cache->ca_flags & flags == 0)
 * timeout are seconds to exit. If timeout reached, the return code will
 * be != 0.
 *
 */
int pgcache_flags_wait( pgcache_t *cache, 
                           uint32_t flags, 
                           int timeout_secs){
    struct timespec start_time, sleep_time, current_time, diff_time;
    int rc = 0;


    clock_gettime( CLOCK_MONOTONIC, &start_time);
    current_time = start_time;
    while(1){
        sleep_time = current_time;
        sleep_time.tv_nsec += NANOSECS_DELAY;
        if( sleep_time.tv_nsec >= MILISECS_PER_SECOND){
            sleep_time.tv_nsec -= MILISECS_PER_SECOND;
            sleep_time.tv_sec++;
        }
        clock_nanosleep( CLOCK_MONOTONIC, TIMER_ABSTIME, &sleep_time, NULL);
        clock_gettime( CLOCK_MONOTONIC, &current_time);

        timespecsub( &current_time, &start_time, &diff_time);
        if( diff_time.tv_sec >= timeout_secs){
            rc = 1;
            break;
        }

        if( ( flags == 0 && cache->ca_flags == 0) || 
            ( flags != 0 && ( ( cache->ca_flags & flags) == flags)) ){
            rc = 0;
            break;
        }
    }

    return( rc);
}

int pgcache_element_flags_wait( pgcache_element_t *el, 
                                    uint32_t flags, 
                                    int timeout_secs){

    struct timespec start_time, sleep_time, current_time, diff_time;
    int rc = 0;


    clock_gettime( CLOCK_MONOTONIC, &start_time);
    current_time = start_time;
    while(1){
        sleep_time = current_time;
        sleep_time.tv_nsec += NANOSECS_DELAY;
        if( sleep_time.tv_nsec >= MILISECS_PER_SECOND){
            sleep_time.tv_nsec -= MILISECS_PER_SECOND;
            sleep_time.tv_sec++;
        }
        clock_nanosleep( CLOCK_MONOTONIC, TIMER_ABSTIME, &sleep_time, NULL);
        clock_gettime( CLOCK_MONOTONIC, &current_time);

        timespecsub( &current_time, &start_time, &diff_time);
        if( diff_time.tv_sec >= timeout_secs){
            rc = 1;
            break;
        }


        pthread_mutex_lock( &el->ce_mutex);

        if( ( flags == 0 && el->ce_flags == 0) || 
            ( flags != 0 && ( ( el->ce_flags & flags) == flags)) ){
            rc = 0;
        }
        pthread_mutex_unlock( &el->ce_mutex);

        if( rc == 0){
            break;
        }
    }

    return( rc);
}


