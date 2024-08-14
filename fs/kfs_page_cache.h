#ifndef _KFS_PGCACHE_H_
#define _KFS_PGCACHE_H_

#include <pthread.h>
#include <stdint.h>
#include "trace.h"

#define KFS_PGCACHE_ND_ACTIVE      0x0001 /* has this element valid data? */
#define KFS_PGCACHE_ND_DIRTY       0x0002 /* dirty? should we sync? */
#define KFS_PGCACHE_ND_EVICT       0x0004 /* remove and free memory */
#define KFS_PGCACHE_ND_PINNED      0x0008 /* dont remove this from cache */
#define KFS_PGCACHE_ND_FORCE_MAP   0x0010 /* evict LRU cache element and 
                                                put this one on place */
#define KFS_PGCACHE_MASK           0x00ff 

typedef struct{
    uint64_t ce_flags;
    void *ce_mem_ptr;   /* ptr to memory */
    uint64_t ce_block_addr;           /* block mapped */
    int ce_num_blocks; /* num of blocks */
    pthread_mutex_t ce_mutex;
    int ce_count; /* number of read/write operations executed */
    void *(*ce_on_unmap_callback)(void *);
}pgcache_element_t;


/* generic cache structure */
typedef struct{
    /* pointer to the elements array */
    pgcache_element_t *ca_elements;

    /* number of elements in use and total */
    uint32_t ca_elements_in_use;
    uint32_t ca_elements_capacity;


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
    pthread_mutex_t ca_mutex; 
    uint16_t ca_flags;
    int ca_nanosec;   /* sleep for N nanosecs */
    int ca_fd;
    pthread_t ca_thread; /* thread */
    pthread_t ca_tid; /* thread id */
}pgcache_t;

pgcache_element_t *kfs_pgcache_element_map( pgcache_t *cache, 
                                            uint64_t addr, 
                                            int numblocks, 
                                            uint32_t flags,
                                            void *(*func)(void *)   );
int kfs_pgcache_alloc( pgcache_t *cache, int fd, int num_elems, int nanosec);
int kfs_pgcache_destroy( pgcache_t *cache);
int kfs_pgcache_start_thread( pgcache_t *cache);
int kfs_pgcache_element_unmap( pgcache_element_t *ce);
int kfs_pgcache_element_mark_dirty( pgcache_element_t *ce);
int kfs_pgcache_sync( pgcache_t *cache);


#endif

