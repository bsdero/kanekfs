#ifndef _KFS_PGCACHE_H_
#define _KFS_PGCACHE_H_

#include <pthread.h>
#include <stdint.h>
#include "trace.h"
#include "cache.h"

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

    pthread_mutex_t ca_mutex; 
    uint16_t ca_flags;
    int ca_fd;
    pthread_t ca_thread; /* thread */
    pthread_t ca_tid; /* thread id */
}pgcache_t;

pgcache_element_t *kfs_pgcache_element_map( pgcache_t *cache, 
                                            uint64_t addr, 
                                            int numblocks, 
                                            uint32_t flags,
                                            void *(*func)(void *)   );

pgcache_t *kfs_pgcache_alloc( int fd, int num_elems);
int kfs_pgcache_destroy( pgcache_t *cache);
int kfs_pgcache_start_thread( pgcache_t *cache);
int kfs_pgcache_element_unmap( pgcache_element_t *ce);
int kfs_pgcache_element_mark_dirty( pgcache_element_t *ce);
int kfs_pgcache_sync( pgcache_t *cache);

int kfs_pgcache_flags_wait( pgcache_t *cache, uint32_t flags, int timeout_secs);
int kfs_pgcache_element_flags_wait( pgcache_element_t *el, 
                                    uint32_t flags, 
                                    int timeout_secs);

#endif

