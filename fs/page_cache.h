#ifndef _PAGE_CACHE_H_
#define _PAGE_CACHE_H_

#include <pthread.h>
#include <stdint.h>
#include "trace.h"
#include "cache.h"

typedef struct{
    cache_element_t pe_el;
    void *pe_mem_ptr;   /* ptr to memory */
    uint64_t pe_block_addr;           /* block mapped */
    int pe_num_blocks; /* num of blocks */
}pgcache_element_t;


/* generic cache structure */
typedef struct{
    cache_t pc_cache;
    int pc_fd;

    /* mutex for this cache */
    pthread_mutex_t pc_mutex;
}pgcache_t;


/* create and init a cache_t structure */
pgcache_t *pgcache_alloc( int fd, int elements_capacity);
int pgcache_destroy( pgcache_t *pgcache);
pgcache_element_t *pgcache_element_map( pgcache_t *cache, 
                                        uint64_t addr, 
                                        int numblocks );

#endif

