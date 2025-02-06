#ifndef _SLOT_CACHE_H_
#define _SLOT_CACHE_H_

#include <pthread.h>
#include <stdint.h>
#include "page_cache.h"

/* For the slots cache the next operations can be done: 
 *
 * map             -open a slot, new or used
 * mark_dirty      -update the slot
 * mark_eviction   -evict the cache element belonging to the slot 
 *                   -not the slot self-
 *
 */


typedef struct{
    pgcache_element_t sc_el;
    uint64_t pe_flags;
    void *pe_mem_ptr;   /* ptr to memory */
    uint64_t pe_block_addr;           /* block mapped */
    int pe_num_blocks; /* num of blocks */
}pgcache_element_t;

typedef struct{
    uint64_t sce_flags;
    pgcache_element_t *sce_index;  /* cached page to slot index */
    pgcache_element_t *sce_extent; /* cached page to slot extent */
    pthread_mutex_t sce_mutex;
    slot_t sce_slot;
}slotcache_element_t;


/* generic cache structure */
typedef struct{
    /* pointer to the elements array */
    slotcache_element_t *sc_elements; 

    pgcache_element_t *sc_slot_map;     /* cached page to the slot map */
    sb_t *sc_sb;                        /* superblock */
    /* number of elements in use and total */
    uint32_t sc_elements_in_use;
    uint32_t sc_elements_capacity;
    pthread_mutex_t sc_mutex; 
    uint16_t sc_flags;
    pthread_t sc_thread; /* thread */
    pthread_t sc_tid; /* thread id */
}slotcache_t;

/* slot cache and slot cache elements operations */
slotcache_t *kfs_slotcache_alloc( sb_t *sb, int num_elems);
int kfs_slotcache_destroy( slotcache_t *slotcache);
int kfs_slotcache_start_thread( slotcache_t *slotcache);

slotcache_element_t *kfs_slotce_new( slotcache_t *sce); 
slotcache_element_t *kfs_slotce_get( slotcache_t *sce, uint64_t slot_id); 
int kfs_slotce_evict( slotcache_element_t *sce);
int kfs_slotce_put( slotcache_element_t *sce);
int kfs_slotce_sync( slotcache_t *sce);



#endif

