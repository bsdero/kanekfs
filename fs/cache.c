#include "cache.h"


typedef struct{


}

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
