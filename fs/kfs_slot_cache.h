#ifndef _KFS_SLOTCACHE_H_
#define _KFS_SLOTCACHE_H_

#include <pthread.h>
#include <stdint.h>
#include "trace.h"
#include "kfs.h"

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
 */

#define KFS_SLCACHE_ND_ACTIVE      0x0001 /* has this element valid data? */
#define KFS_SLCACHE_ND_DIRTY       0x0002 /* dirty? should we sync? */
#define KFS_SLCACHE_ND_EVICT       0x0004 /* remove and free memory */
#define KFS_SLCACHE_ND_PINNED      0x0008 /* dont remove this from cache */
#define KFS_SLCACHE_ND_FORCE_MAP   0x0010 /* evict LRU cache element and 
                                                put this one on place */
#define KFS_SLCACHE_ND_CLEAN       0x0020 /* this flag should be enabled only
                                             if the node is active and NOT
                                             dirty */
#define KFS_SLCACHE_MASK           0x00ff 


typedef struct{
    uint64_t sce_flags;
    pgcache_element_t *sce_index;
    pgcache_element_t *sce_extent;
    pthread_mutex_t sce_mutex;
    slot_t sce_slot;
}slot_cache_element_t;


/* generic cache structure */
typedef struct{
    /* pointer to the elements array */
    pgcache_element_t *sc_slot_map;     /* cached page to the slot map */
    sb_t *sc_sb;                        /* superblock */

    /* number of elements in use and total */
    uint32_t sc_in_use;
    uint32_t sc_capacity;


#define KFS_PGCACHE_READY          0x01 /* read flag. If enabled,
                                           the cache is ready for run */
#define KFS_PGCACHE_ACTIVE         0x02 /* read flag. if enabled, 
                                         * processing loop may run any 
                                         * time */
#define KFS_PGCACHE_ON_LOOP        0x04 /* read flag, if enabled 
                                         * the processing loop is 
                                         * running */
#define KFS_PGCACHE_FLUSH          0x08 /* set this for flush all the
                                           buffers in cache */
#define KFS_PGCACHE_PAUSE_LOOP     0x10 /* set this for pause the loop,
                                           clear it for un pause */

#define KFS_PGCACHE_EXIT_LOOP      0x20 /* set this for exit the loop
                                           flush out buffers, and 
                                           evict all */
#define KFS_PGCACHE_EVICTED        0x40 /* if enabled, eveything has 
                                           been evicted, out now */

    pthread_mutex_t sc_mutex; 
    uint16_t sc_flags;
    pthread_t sc_thread; /* thread */
    pthread_t sc_tid; /* thread id */
}slot_cache_t;


pgcache_t *kfs_slotcache_alloc( sb_t *sb, int num_elems);
int kfs_slotcache_destroy( pgcache_t *cache);
int kfs_slotcache_start_thread( pgcache_t *cache);

slot_cache_element_t *kfs_slot_cel_new( pgcache_t *slotcache); 
slot_cache_element_t *kfs_slot_cel_get( pgcache_t *slotcache, 
                                        uint64_t slot_id); 
int kfs_slot_cel_evict( slot_cache_element_t *sce);
int kfs_slot_cel_put( slot_cache_element_t *sce);
int kfs_slot_cel_sync( pgcache_t *cache);


#endif

