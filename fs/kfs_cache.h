#ifndef _KFS_CACHE_H_
#define _KFS_CACHE_H_



#define KFS_CACHE_ACTIVE         0x0001 /* is this element in cache? */
#define KFS_CACHE_DIRTY          0x0002 /* dirty? should we sync? */
#define KFS_CACHE_EVICT          0x0004 /* remove and free memory */
#define KFS_CACHE_PINNED         0x0008 /* dont remove this from cache */
#define KFS_CACHE_FLUSH          0x0010 /* flush to block dev */

#define KFS_CACHE_MASK           0x00ff 

typedef struct{
    void *ce_ptr;

    int ce_flags;
    time_t ce_u_time;
    uint64_t ce_addr;           /* block mapped */
    int ce_num_blocks; /* num of blocks */
}cache_element_t;


/* generic cache structure */
typedef struct{
    /* pointer to the elements array */
    void *ca_elements;

    /* number of elements in use and total */
    uint32_t ca_elements_in_use;
    uint32_t ca_elements_capacity;

    /* last update time */
    time_t ca_u_time;

#define KFS_CACHE_DT_LIST        0x01
#define KFS_CACHE_DT_BUF         0x02
    uint16_t ca_flags;
    int ca_fd;
}cache_t;


int kfs_cache_init( cache_t *cache, int fd, int num_elems);
int kfs_cache_destroy( cache_t *cache);
cache_element_t *kfs_cache_map( cache_t *cache, uint32_t addr);
int kfs_cache_unmap( cache_t *cache, cache_element_t *cache);

#endif

