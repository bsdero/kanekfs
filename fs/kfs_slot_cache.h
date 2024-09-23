#ifndef _KFS_SLOTCACHE_H_
#define _KFS_SLOTCACHE_H_

#include <pthread.h>
#include <stdint.h>
#include "trace.h"
#include "cache.h"

/* For the slots the next operations can be done: 
 *
 * reserve         -looks for an empty slot and open it
 * get             -open an existing slot
 * put             -close an existing slot
 * evict           -evict an existing slot
 *
 * once the slots are open, the dict can be read or written. 
 * The operations when the slots are open:
 * update          -dict get updated 
 * The operations are exclusive, and the slot are locked. 
 *
 * For the cache: 
 * reserve:        -looks for an empty slot in the slot map, update and 
 *                  dirtymark
 *                 -load the slot index block into the page cache
 *                 -update superblock with number of slots
 *
 * get             -load the slot index block of the requested slot id into
 *                  the page cache
 *                 -load the page cache of the dict extent, load the dict
 *                  
 * update and put  -if the dict fits into the dict extent, just update and 
 *                  mark the extent as dirty.
 *                  If it does not, reserve an extent in the block map, load 
 *                  into the page cache, update, mark as dirty. 
 *                  Then update the slot in the cached index extent, mark as 
 *                  dirty 
 *                  blank the old dict extent, mark as dirty and free the
 *                  extent in the map
 *                  
 * evict           -update dict index, blank dict extent, mark both as dirty
 *                 -update number of available slots in superblock, and put it
 *                 -update slot map and block map
 *
 *
 */

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

