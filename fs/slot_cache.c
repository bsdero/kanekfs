#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <sys/time.h>
#include "trace.h"
#include "slots.h"
#include "slot_cache.h"

void *slcache_on_evict( void *arg){
    slcache_element_t *slcache_el = ( slcache_element_t *) arg;
    if( slcache_el->sce_slot != NULL){
        slot_destroy( pgcache_el->sce_slot);
    }
    return( NULL);
}



void *pgcache_on_flush( void *arg){
    slcache_element_t *slcache_el = ( slcache_element_t *) arg;
    slcache_t *slcache; 

    slcache = (pgcache_t *) slcache_el->pe_el.ce_owner_cache;

    rc = slot_save( slcache->sc_pgcache, 
                    slot_id, 
                    slcache_el->sce_slot);

    if( rc != 0){
        TRACE_ERR("extent write error");
    } 
    return( NULL);
}




slcache_element_t *slcache_slot_map( slcache_t *slcache, 
                                     uint64_t slot_id){
    int i, rc;
    slcache_element_t *slot_el;
    cache_element_t *el;
    cache_t *cache = (cache_t *) slcache;
    slot_t *slot;
    void *p;
    
    TRACE("start");


    /* loop in the cache, look if the slot is there already */
    pthread_mutex_lock( &slcache->sc_mutex);
    for( i = 0; i < cache->ca_elements_capacity; i++){
        cel = cache->ca_elements_ptr[i];

        if( cel == NULL){
            continue;
        }
        if( cel->ce_id == slot_id){ /* element exist */
            CACHE_EL_ADD_COUNT( cel);
            el = (slcache_element_t *) cel;
            goto exit0;
        }
    }

    /* map cache element */
    p = cache_element_map( cache, sizeof( slcache_element_t));
    if( p == NULL){
        el = NULL;
        goto exit0;
    }

    /* init slot mutex */
    el = ( cache_element_t *) p;
    slot_el = ( slcache_element_t *) p;

    slot_el->sce_slot = slot_load( slcache->sc_pgcache, slot_id);
    if( slot_el->sce_slot == NULL){
        cache_element_mark_eviction( CACHE_EL( el));
        TRACE_ERR("Could not load slot %ul", slot_id);
        goto exit0;
    }

exit0:
    pthread_mutex_unlock( &slcache->sc_mutex);
    
    TRACE("end");
    return( el);
}


slcache_element_t *slcache_slot_alloc( slcache_t *slcache){
    int i, rc;
    slcache_element_t *slot_el;
    cache_element_t *el;
    cache_t *cache = (cache_t *) slcache;
    slot_t *slot;
    void *p;
    
    TRACE("start");

    /* map cache element */
    p = cache_element_map( cache, sizeof( slcache_element_t));
    if( p == NULL){
        el = NULL;
        goto exit0;
    }

    /* init slot mutex */
    el = ( cache_element_t *) p;
    slot_el = ( slcache_element_t *) p;

    slot_el->sce_slot = slot_alloc( slcache->sc_pgcache);
    if( slot_el->sce_slot == NULL){
        cache_element_mark_eviction( CACHE_EL( el));
        TRACE_ERR("Could not load slot %ul", slot_id);
        goto exit0;
    }

exit0:
    pthread_mutex_unlock( &slcache->sc_mutex);
    
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
    pthread_mutex_destroy( &slcache->sc_mutex);
    cache_disable( &slcache->sl_cache);
    free( slcache);
    TRACE("end");

    return(0);
}




