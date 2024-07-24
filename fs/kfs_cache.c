#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "kfs_cache.h"



int kfs_cache_init( cache_t *cache, int fd, int num_elems){
    memset( (void *) cache, 0, sizeof( cache_t));

}

int kfs_cache_destroy( cache_t *cache);
cache_element_t *kfs_cache_map( cache_t *cache, uint32_t addr);
int kfs_cache_unmap( cache_t *cache, cache_element_t *cache);
int kfs_cache_iter( cache_t *cache);



