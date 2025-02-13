#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <sys/time.h>
#include "trace.h"
#include "page_cache.h"
#include "eio.h"
#include "kfs_mem.h"



void *pgcache_on_evict( void *arg){
    pgcache_element_t *pgcache_el = ( pgcache_element_t *) arg;
    free( pgcache_el->pe_mem_ptr);
    return( NULL);
}

void *pgcache_on_flush( void *arg){
    pgcache_element_t *pgcache_el = ( pgcache_element_t *) arg;
    pgcache_t *pgcache; 

    pgcache = (pgcache_t *) pgcache_el->pe_el.ce_owner_cache;

    int rc = extent_write( pgcache->pc_fd, 
                           pgcache_el->pe_mem_ptr, 
                           pgcache_el->pe_block_addr, 
                           pgcache_el->pe_num_blocks);

    if( rc != 0){
        TRACE_ERR("extent write error");
    } 
    return( NULL);
}




pgcache_element_t *pgcache_element_map( pgcache_t *pgcache, 
                                        uint64_t addr, 
                                        int numblocks){
    int i, rc;
    pgcache_element_t *el;
    cache_element_t *cel;
    cache_t *cache = (cache_t *) pgcache;
    void *p;
    
    TRACE("start");


    /* loop in the cache, look if the elements is there already */
    pthread_mutex_lock( &pgcache->pc_mutex);
    for( i = 0; i < cache->ca_elements_capacity; i++){
        cel = cache->ca_elements_ptr[i];

        if( cel == NULL){
            continue;
        }
        if( cel->ce_id == addr){ /* element exist */
            CACHE_EL_ADD_COUNT( cel);
            el = (pgcache_element_t *) cel;
            if( numblocks <= el->pe_num_blocks){ /* page already mapped, 
                                                  * with the size we need, 
                                                  * just return. */
                goto exit0;
            }else{ /* same address, but requires more blocks. So we will
                      lock, flush, realloc, re-read and unlock. */
                pthread_mutex_lock( &cel->ce_mutex); /* lock element */
                if( (flags & CACHE_EL_DIRTY) != 0){
                    pg_cache_on_flush( cel); /* flush if needed */
                }

                /* realloc */ 
                el->pe_mem_ptr = realloc( numblocks * KFS_BLOCKSIZE);

                if( el->pe_mem_ptr == NULL){
                    TRACE_ERR("malloc error");
                    el = NULL;
                    goto exit0;
                }

                rc = extent_read( pgcache->pc_fd, 
                                  el->pe_mem_ptr, 
                                  addr, 
                                  numblocks);
                if( rc != 0){
                    TRACE_ERR("extent read error");
                    free( el->pe_mem_ptr);
                    el = NULL;
                    goto exit0;
                }
                el->pe_num_blocks = numblocks;
                pthread_mutex_unlock( &cel->ce_mutex); /* unlock element */
                goto exit0;
            }
            break;
        }
    }

    /* if we are here, the required page is not cached. Lets fix that. */
    /* map cache element */
    p = cache_element_map( CACHE( pgcache), 
                           sizeof( pgcache_element_t));
    if( p == NULL){
        el = NULL;
        goto exit0;
    }
    
    el = ( pgcache_element_t *) p;
    el->pe_mem_ptr = malloc( numblocks * KFS_BLOCKSIZE);
    if( el->pe_mem_ptr == NULL){
        TRACE_ERR("malloc error");
        el = NULL;
        goto exit0;
    }


    rc = extent_read( pgcache->pc_fd, el->pe_mem_ptr, addr, numblocks);
    if( rc != 0){
        TRACE_ERR("extent read error");
        free( el->pe_mem_ptr);
        el = NULL;
        goto exit0;
    }

    el->pe_block_addr = addr;
    el->pe_num_blocks = numblocks;
    el->pe_el.ce_id = (uint64_t) addr;
 
exit0:
    pthread_mutex_unlock( &pgcache->pc_mutex);
    
    TRACE("end");
    return( el);
}



pgcache_element_t *pgcache_element_map_zero( pgcache_t *pgcache, 
                                             uint64_t addr, 
                                             int numblocks){
    int i, rc;
    pgcache_element_t *el;
    cache_element_t *cel;
    cache_t *cache = (cache_t *) pgcache;
    void *p;
    
    TRACE("start");


    /* loop in the cache, look if the elements is there already */
    pthread_mutex_lock( &pgcache->pc_mutex);
    for( i = 0; i < cache->ca_elements_capacity; i++){
        cel = cache->ca_elements_ptr[i];

        if( cel == NULL){
            continue;
        }
        if( cel->ce_id == addr){ /* element exist, not good to map zeroes
                                  * blocks into an existing cache element
                                  */
            CACHE_EL_ADD_COUNT( cel);
            TRACE("page cached already, addr=0x%x,%lu", addr, addr);
            el = NULL;
            goto exit0;
            break;
        }
    }

    /* map cache element */
    p = cache_element_map( CACHE( pgcache), 
                           sizeof( pgcache_element_t));
    if( p == NULL){
        el = NULL;
        goto exit0;
    }
    
    el = ( pgcache_element_t *) p;
    el->pe_mem_ptr = malloc( numblocks * KFS_BLOCKSIZE);
    if( el->pe_mem_ptr == NULL){
        TRACE_ERR("malloc error");
        el = NULL;
        goto exit0;
    }

    memset( el->pe_mem_ptr, 0,  numblocks * KFS_BLOCKSIZE);
    el->pe_block_addr = addr;
    el->pe_num_blocks = numblocks;
    el->pe_el.ce_id = (uint64_t) addr;
 
exit0:
    pthread_mutex_unlock( &pgcache->pc_mutex);
    
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
                     pgcache_on_evict,
                     pgcache_on_flush);
    if( rc != 0){
        TRACE_ERR("pgcache->cache init has failed"); 
        free( pgcache);
        pgcache = NULL;
        goto exit0;
    }


    if( pthread_mutex_init( &pgcache->pc_mutex, NULL) != 0) { 
        TRACE_ERR("mutex init has failed");
        free( pgcache);
        pgcache = NULL;
        goto exit0;
    }

    pgcache->pc_fd = fd;

exit0:
    TRACE("end");

    return( pgcache);
}



int pgcache_destroy( pgcache_t *pgcache){
    TRACE("start");
    cache_disable( &pgcache->pc_cache);
    free( pgcache);
    TRACE("end");

    return(0);
}

/* locking call, alloc a cache, start it, and wait for it to be running. */
int pgcache_enable_sync( pgcache_t *pgcache){
    int rc = 0; 
    rc = cache_enable( CACHE(pgcache) );
    if( rc != 0){
        TRACE_ERR("Issues in cache_enable()");
        goto exit1;
    }
        
    rc = cache_wait_for_flags( CACHE( pgcache), 
                               CACHE_ACTIVE, 
                               PGCACHE_DEFAULT_TIMEOUT);
    if( rc != 0){
        TRACE_ERR("timeout waiting for CACHE_ACTIVE flag");
    }

exit1:
    return( rc);
}

/* pgcache sync map */
pgcache_element_t *pgcache_element_map_sync( pgcache_t *pgcache, 
                                             uint64_t addr, 
                                             int numblocks ){
    pgcache_element_t *el;
    int rc = 0;

    el = pgcache_element_map( pgcache, 0, 1);
    if( el == NULL){
        TRACE_ERR("Issues in pgcache_element_map()");
        goto exit1;
    }


    rc = cache_element_wait_for_flags( CACHE_EL(el), 
                                       CACHE_EL_ACTIVE, 
                                       PGCACHE_DEFAULT_TIMEOUT);
    if( rc != 0){
        cache_element_mark_eviction( CACHE_EL(el));
        el = NULL;
        TRACE_ERR("timeout waiting for CACHE_EL_ACTIVE");
    }
exit1:
    return( el);
}

/* pgcache sync */
int pgcache_element_sync( pgcache_element_t *el){ 
    int rc = 0;

    rc = PGCACHE_EL_MARK_DIRTY( el);
    if( rc != 0){
        TRACE_ERR("Issues in pgcache_element_mark_dirty()");
        goto exit1;
    }


    rc = cache_element_wait_for_flags( CACHE_EL(el), 
                                       CACHE_EL_CLEAN, 
                                       PGCACHE_DEFAULT_TIMEOUT);
    if( rc != 0){
        TRACE_ERR("timeout waiting for CACHE_EL_CLEAN");
    }
exit1:
    return( rc);
}


