#ifndef _KFS_MEM_H_
#define _KFS_MEM_H_

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <time.h>
#include <ctype.h>
#include "trace.h"
#include "dict.h"



#define KFS_CACHE_LOADED         0x0001 /* ex_addr has valid data */
#define KFS_CACHE_IN_CACHE       0x0002 /* is this element in cache? */
#define KFS_CACHE_DIRTY          0x0004 /* dirty? should we sync? */
#define KFS_CACHE_EVICT          0x0008 /* remove and free memory */
#define KFS_CACHE_ACTIVE         0x0010 /* is this data structure in use? */
#define KFS_CACHE_PINNED         0x0020 /* dont remove this from cache */
#define KFS_CACHE_FLUSH          0x0040 /* flush to block dev and evict */

#define KFS_CACHE_MASK           0x00ff 

/* this structure maps an extent or segment of an extent in the memory */
typedef struct{
    unsigned char *ex_addr; /* extent in memory */
    uint32_t size; /* how much of extent is in memory */
    uint64_t offset; /* offset of the real extent */
    /* data for locate in disk */
    uint64_t ex_block_addr; /* first logical block extent covers */
    uint16_t ex_block_size; /* number of blocks covered by extent */
    uint16_t ex_log_size; /* number of logical units */
    uint32_t ex_log_addr; /* offset in logical units */

    /* flags for identify which kind of data has this extent */
#define KFS_EXT_DATA             0x0100 
#define KFS_EXT_EDGES            0x0200
#define KFS_EXT_SLOTS            0x0400
#define KFS_EXT_SUPERBLOCK       0x0800
#define KFS_EXT_SUPERINODES      0x1000
#define KFS_EXT_BITMAP           0x2000
#define KFS_EXT_DEFAULT          0x4000
    uint32_t ex_flags;

    /* last update time */
    time_t ex_u_time;

    /* we should add callbacks for advice the objects owner of this extent
     * when something happened: an eviction, a flush or whatever */
}map_extent_t;

/* slots and slot table definition */
typedef struct{
    uint64_t slot_id;
    uint64_t slot_sino_owner;
    uint32_t slot_link_owner;
     
#define SLOT_OWNER_SB            0x0000
#define SLOT_OWNER_INO           0x0001
#define SLOT_OWNER_LINK          0x0002
#define SLOT_OWNER_MASK          0x000f

#define SLOT_IN_USE              0x0100
#define SLOT_UPDATE              0x1000
    uint32_t slot_flags;
    dict_t slot_d;
    map_extent_t *extent;
}slot_t;




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

}cache_t;



typedef struct{
    uint64_t ed_to_super_node; /* which node is this edge pointing to */
    uint64_t ed_hash_name;     /* hash79 */

#define KFS_EDGE_IS_DENTRY       0x0100 /* is this edge traversable? */
#define KFS_EDGE_IS_VISIBLE      0x0200 /* is this edge visible? */
#define KFS_EDGE_FULL_CHECK      0x0400 /* hash is not enough, check name */
#define KFS_EDGE_SUBINDEX_MASK   0x0001 /* mask for subindex of this edge */     

    uint16_t ed_flags;  /* flags for this edge */
    char *ed_name;
}edge_t;

typedef struct{
    uint64_t li_id;
    uint64_t li_slot_id;
    edge_t edges[2];
}link_t;


typedef struct{
    edge_t **si_edges; /* array of edges of ptrs */

    uint64_t si_slot_id; /* slot ID */
    time_t si_a_time;
    time_t si_c_time;
    time_t si_m_time;

    uint64_t si_id; /* super inode unique ID */
    uint64_t si_data_len; /* file size */


    map_extent_t *edges, *data;

    uint32_t si_edges_num;  /* total num of edges */
    

    /* cache flags may be used here */
    uint32_t si_flags;
    uint32_t si_gid;
    uint32_t si_uid;


    uint32_t si_mode; 
    uint32_t si_count;
}sinode_t; 




/* extents, slots, super inodes and graph entries should be cacheables */


/* Graph cache element */
typedef struct{
    edge_t **nd_edges;
    uint32_t nd_edges_num;
    uint8_t nd_visited[12]; /* 96 bits for graph searches */
    uint64_t nd_slot_id; 
    sinode_t *nd_sinode;
    time_t nd_lru;
}node_t;


typedef struct{
    uint64_t pc_path_hash;
    uint64_t pc_sinode_start;
    uint64_t pc_sinode_end;
    edge_t **pc_path;
    time_t pc_lru;
    uint32_t pc_count;
}path_cache_t;


typedef struct{
    uint64_t t_capacity;
    uint64_t t_in_use;
    map_extent_t *t_extent, *t_bitmap;
    cache_t *t_cache;
}table_t;


typedef struct{
    uint64_t sb_root_super_inode; /* any inode can be the root inode */

    /* extents cache */
    cache_t *sb_extents_cache;

    /* super inodes capacity, used inodes, cache, and extents */
    table_t sb_sinodes;

    /* slots capacity, used, cache and extents */
    table_t sb_slots;

    /* bit map capacity in blocks, taken and extents. Cache is not used. */
    table_t sb_blockmap;

    time_t sb_c_time, sb_m_time, sb_a_time; 

    uint64_t flags;

    /* block dev */
    int dev;
}sb_t;


    
/* super block operations */
void kfs_sb_statfs();
void kfs_sb_sl_table_dump();
void kfs_sb_si_table_dump();
sinode_t kfs_sb_alloc_sinode(); /* alloc and fill in a super inode */
void kfs_sb_destroy_sinode( sinode_t *sinode); /* undo whatever done in
                                                kfs_sb_alloc_sinode */
slot_t kfs_sb_alloc_slot(); /* alloc and fill in a slot */
void kfs_sb_detroy_slot( slot_t *slot); /* undo whatever done in 
                                            kfs_sb_alloc_slot */
void kfs_sb_sync();
void kfs_sb_put_super();
void kfs_sb_get_super( char *file_name);
void kfs_sb_evict_inode( sinode_t *sinode);
void kfs_sb_evict_slot( slot_t *slot);


/* operations with slots */
slot_t kfs_slot_alloc();
slot_t kfs_slot_new();
slot_t kfs_slot_get( uint64_t slot_id);
slot_t kfs_slot_get_locked( uint64_t slot_id);

void slot_set_in_use( slot_t *s, int in_use);
void slot_set_owner_inode( slot_t *s, uint64_t inode);
void slot_set_owner_inode_edge( slot_t *s, uint64_t inode, uint64_t edge);
void slot_set_owner_inode_edge_s( slot_t *s, uint64_t inode, char *es);
void slot_set_dict( slot_t *s, dict_t d);

void kfs_slot_dump( slot_t *slot);
int kfs_slot_insert_cache( slot_t *slot);
int kfs_slot_dirty( slot_t *slot);
int kfs_slot_put( slot_t *slot);
int kfs_slot_write( slot_t *slot);
int kfs_slot_evict( slot_t *slot);

/* Operations with super inodes */
sinode_t kfs_sinode_alloc();
sinode_t kfs_sinode_new();
sinode_t kfs_sinode_get( uint64_t sinode_id);
sinode_t kfs_sinode_get_locked( uint64_t sinode_id);
int kfs_sinode_insert_cache( sinode_t *sinode);
int kfs_sinode_dirty( sinode_t *sinode);
int kfs_sinode_put( sinode_t *sinode);
int kfs_sinode_write( sinode_t *sinode);
int kfs_sinode_evict( sinode_t *sinode);




#endif

