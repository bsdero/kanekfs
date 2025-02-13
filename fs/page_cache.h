#ifndef _PAGE_CACHE_H_
#define _PAGE_CACHE_H_

#include <pthread.h>
#include <stdint.h>
#include "trace.h"
#include "cache.h"


#define PGCACHE_DEFAULT_TIMEOUT          10 /* timeout in seconds */

typedef struct{
    cache_element_t pe_el;
    void *pe_mem_ptr;   /* ptr to memory */
    uint64_t pe_block_addr;           /* block mapped */
    int pe_num_blocks; /* num of blocks */
}pgcache_element_t;


typedef struct{
    cache_t pc_cache;
    int pc_fd;

    /* mutex for this cache */
    pthread_mutex_t pc_mutex;
}pgcache_t;


/* create and init a page cache_t structure */
pgcache_t *pgcache_alloc( int fd, int elements_capacity);
int pgcache_destroy( pgcache_t *pgcache);

/* map a number of blocks from the device into memory */
pgcache_element_t *pgcache_element_map( pgcache_t *cache, 
                                        uint64_t addr, 
                                        int numblocks );

/* get numblocks from memory and map to device. 
 * Basically is the same than pgcache_element_map() but not reading 
 * at first from disk, and mapping blank blocks to the specified address */
pgcache_element_t *pgcache_element_map_zero( pgcache_t *cache,
                                             uint64_t addr,
                                             int numblocks);


/* locking call, alloc a cache, start it, and wait for it to be running. */
int pgcache_enable_sync( pgcache_t *pgcache);

/* pgcache sync map */
pgcache_element_t *pgcache_element_map_sync( pgcache_t *cache, 
                                             uint64_t addr, 
                                             int numblocks );

/* pgcache element sync */
int pgcache_element_sync( pgcache_element_t *el);

#define PGCACHE_EL_MARK_DIRTY(x)    cache_element_mark_dirty( \
                                             CACHE_EL(x));
 

#endif

