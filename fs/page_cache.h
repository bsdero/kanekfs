#ifndef _KFS_PGCACHE_H_
#define _KFS_PGCACHE_H_

#include <pthread.h>
#include <stdint.h>
#include "trace.h"
#include "cache.h"

typedef struct{
    cache_element_t pe_el;
    uint64_t pe_flags;
    void *pe_mem_ptr;   /* ptr to memory */
    uint64_t pe_block_addr;           /* block mapped */
    int pe_num_blocks; /* num of blocks */
}pgcache_element_t;


/* generic cache structure */
typedef struct{
    cache_t pc_cache;
    int pc_fd;
}pgcache_t;


/* create and init a cache_t structure */
pgcache_t *pgcache_alloc( int fd, int elements_capacity);
#define pgcache_run(x)         (cache_run( CACHE(x)))
int pgcache_destroy( pgcache_t *pgcache);
#define pgcache_sync(x)        (cache_sync( CACHE(x)))


pgcache_element_t *pgcache_element_map( pgcache_t *cache, 
                                        uint64_t addr, 
                                        int numblocks );

#endif

