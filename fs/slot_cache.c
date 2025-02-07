#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <sys/time.h>
#include "trace.h"
#include "slot_cache.h"

/* NEED SOME WORK */
void *slcache_on_evict( void *arg){
    pgcache_element_t *slcache_el = ( pgcache_element_t *) arg;
    free( pgcache_el->pe_mem_ptr);
    return( NULL);
}



/* NEED SOME WORK */
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




/* NEED SOME WORK */
slcache_element_t *slcache_element_map( slcache_t *slcache, 
                                        uint64_t slot_id){
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
            if( numblocks == el->pe_num_blocks){ /* page already mapped */
                goto exit0;
            }else{ /* same address, different size, evict */
                cache_element_evict( cache, addr);
                el = NULL;
                cel = NULL;
            }
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


/* create and init a slot cache_t structure. Use an already existing 
 * page cache. 
 *
 * */

slcache_t *slcache_alloc( pg_cache_t *pg_cache,
                          int elements_capacity){
    slcache_t *sl_cache;
    int rc;

    TRACE("start");

    slcache = malloc( sizeof( slcache_t ));
    if( slcache == NULL){
        TRACE_ERR("Error in malloc()"); 
        goto exit0;
    }

    memset( (void *) slcache, 0, sizeof( slcache_t));
    rc = cache_init( &slcache->sc_cache, 
                     elements_capacity,
                     slcache_on_evict,
                     slcache_on_flush);
    if( rc != 0){
        TRACE_ERR("slcache->sc_cache init has failed"); 
        free( slcache);
        slcache = NULL;
        goto exit0;
    }


    if( pthread_mutex_init( &slcache->sc_mutex, NULL) != 0) { 
        TRACE_ERR("mutex init has failed");
        free( slcache);
        slcache = NULL;
        goto exit0;
    }

    slcache->sc_pgcache = pg_cache;


exit0:
    TRACE("end");

    return( sccache);
}



int pgcache_destroy( slcache_t *slcache){
    TRACE("start");
    cache_disable( &slcache->sl_cache);
    free( slcache);
    TRACE("end");

    return(0);
}




