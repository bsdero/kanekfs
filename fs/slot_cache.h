#ifndef _SLOT_CACHE_H_
#define _SLOT_CACHE_H_

#include <pthread.h>
#include <stdint.h>
#include "page_cache.h"
#include "kfs_config.h"

/* For the slots cache the next operations can be done: 
 *
 * map             -open a slot, new or used
 * mark_dirty      -update the slot
 * mark_eviction   -evict the cache element belonging to the slot 
 *                   -not the slot self-
 *
 */


typedef struct{
    pgcache_element_t *sce_index;  /* cached page to slot index */
    pgcache_element_t *sce_extent; /* cached page to slot extent */
    pthread_mutex_t sce_mutex;
    slot_t *sce_slot;
}slcache_element_t;


typedef struct{
    cache_t sc_cache; /* slots cache */
    pgcache_t *sc_pgcache;  /* page cache */

    /* mutex for this cache */
    pthread_mutex_t sc_mutex;
}slcache_t;



/* create and init a slot cache_t structure */
slcache_t *slcache_alloc( pg_cache_t *pg_cache, 
                          int elements_capacity);
int slcache_destroy( slcache_t *slcache);
slcache_element_t *slcache_element_map( slcache_t *slcache, uint64_t slot_id);
slcache_element_t *slcache_element_alloc( slcache_t *slcache);


#endif

