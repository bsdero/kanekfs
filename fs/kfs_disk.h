#ifndef _KFS_DISK_H_
#define _KFS_DISK_H_

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


#define KFS_MAGIC                                  0x1ba7ba  
#define KFS_BLOCK_SIZE                             8192

/* Thses are the on-disk structures. To support them, we need some headers 
 * for store information which can be numbered, like slots, inodes, edges, 
 * or extents.
 *
 * The kfs_extent_header_t is a data structure designed for descript how is 
 * data stored in the remaining bytes of this block, or in an group of 
 * extents.
 *
 * After a kfs_extent_header_t, we may have kfs_extent_entry_t entries or 
 * another struct descripting a slots/sinodes table or both. Depends on 
 * what are we using the extent for, and we know what to read an how depending
 * on the value in the field 'magic'. 
 *
 * So, we can store data directly in the same block or have single, double, 
 * triple or nth indirect block pointers, which points to extents, conforming
 * a tree. We can also have a mix, to save disk space. 
 *
 * So, how the kfs_extent_header_t data structure works?
 *
 * Simple we always have an initial root block. It may store data, so it 
 * is a leaf block also. If that's the case, no more data is expected than 
 * we have stored in the same block, and thats all. No pointer, no extents, 
 * no indirect indexing, nothing. The leafs blocks are also data 
 * terminators. 
 *
 * But the root may reference data in extents of blocks, so it can act as 
 * an index block. We can have also an index block with pointers to other 
 * index blocks too.
 *
 * In some cases, the root block may have a single extet_entry
 *
 * The final blocks or extents, where data is stored are called a "leaf".
 * Leafs may have organized data as we want, all depends on the "magic"
 * field of the root block kfs_extent_header_t. No index or pointers 
 * are expected in a "leaf" block/extent. 
 *
 * So, how can we use the extent_header?
 * 
 * 1.-If this is the data root, the flag KFS_ENTRIES_ROOT is enabled.  
 * 2.-If this is a leaf -i.e., we have data in the same block-, the flag
 *    KFS_ENTRIES_LEAF is enabled and eh_depth_tree_level = 0. 
 *    The eh_entries_in_use and eh_entries_capacity descripts how many 
 *    elements we can store and how many are used in this block extent. 
 *    No more data than stored in this block/extent is expected. 
 * 3.-If entries are in other blocks/extents, the flag KFS_ENTRIES_INDEX is 
 *    enabled and eh_depth_level_is > 0. Like the leafs, the eh_entries_in_use
 *    and eh_entries_capacity fields descripts how many indexing entries
 *    we may store and how many are used in this block extent. 
 * 4.-Additional data structures descripting data and possible table 
 * headers are inmediately after the kfs_table_header_t.
 * 
 * All the blocks/extents used for store tables data must have a table_header. 
 */
typedef struct{

/* those magics define what are we dealing with */
#define KFS_EXTENT_MAGIC                           0xaca771

    
#define KFS_SLOTS_ROOT_MAGIC                       0x0c01a7e
#define KFS_SLOTS_BITMAP_MAGIC                     0x1a77e
#define KFS_SLOTS_TABLE_MAGIC                      0xdecaf
#define KFS_SLOTS_DATA_MAGIC                       0xc0ffee

#define KFS_SINODE_ROOT_MAGIC                      0xc001beb
#define KFS_SINODE_BITMAP_MAGIC                    0xbad50da
#define KFS_SINODE_TABLE_MAGIC                     0xcafe
#define KFS_SINODE_DATA_MAGIC                      0xc0cac01a

#define 

    uint32_t eh_magic; /* table type */
    
    uint32_t eh_entries_in_use; /* number of used entries here */
    uint32_t eh_entries_capacity; /* max capacity of entries */

#define KFS_ENTRIES_ROOT                           0x000001    
#define KFS_ENTRIES_INDEX                          0x000002
#define KFS_ENTRIES_LEAF                           0x000004
    /* we can have extra flags, depending on what are we storing */
    uint16_t eh_flags;
    uint16_t eh_depth_tree_level; /* how many levels are in the tree below */
    uint64_t eh_blocks_count;
}kfs_extent_header_t; 

/*
 * This is the extent on-disk structure. It points to other index or leaf 
 * blocks/extents. */
typedef struct{
    uint64_t ee_block_addr; /* first logical block extent covers */
    uint16_t ee_blocks_size; /* number of blocks covered by extent */
    uint16_t ee_logical_size; /* number of logical units */
    uint32_t ee_log_addr; /* offset in logical units */
}kfs_extent_entry_t;




/* All the metadata for nodes and edges in the graph are stored in 
 * "key-values" and organized in "slots". For this effect, each slot has a 
 * unique ID -which for our purposes is a sequencial unsigned integer-, and 
 * an array of key-values -which I will call, a dictionary-, like this:
 *  ----------------------------------------------------------------------
 *    SLOT 0: [ key1=value1, k2=myval ]
 *    SLOT 1: []
 *    SLOT 2: [ mykey=0, publickey="8w09cfw432f09", distance=32.6435
 *    SLOT 3: [ mount_host29d9i3="{n:32:localhost:97}", lock="ef4987s2" ]
 *    SLOT 4: [ Editor="armando" ]
 *  ----------------------------------------------------------------------
 *  I am familiar with python, so basically we have an array of dictionaries
 *  here. So, the next data structures provides an on-disk representation 
 *  for store this data. 
 *
 * For ease of locate for empty slots, a slot index is provided. 
 * Once a slot is located in the slot index, the slot dictionary can be 
 * read, updated or removed. 
 *
 * STORAGE:
 *
 * We have a Slot Table stored in indirect extents. And for each slot, we may 
 * have a single block or an extent to store slot/dictionary data. 
 *
 * So, initially we have an extent header with the magic KFS_SLOTS_TABLE_MAGIC
 * followed by a kfs_metadata_descriptor_t. After that, we have extent_entries
 * filling the whole block ,which is an index block with pointers to extents 
 * for the slots table, which may grow on demand. 
 */

/* kfs_metadata_descriptor_t is a data structure on disk, which, together with
 * a kfs_table_header_t structure in an extent root block, keeps extra data 
 * of the slots  -dictionaries- stored.
 * It should allow:
 *   -Creation and population into dictionaries slots
 *   -Read data from slots
 *   -Updates of data in slots
 *   -Deletion of data in slots
 *   -Provisioning of extra slots.
 *
 *   The kfs_metadata_descriptor_t should be stored inmediately after the 
 *   kfs_extent_header_t with the magic KFS_SLOTS_TABLE_MAGIC.
 */
typedef struct{
    uint64_t total_slots_count;        /* slots capacity */
    uint64_t total_slots_in_use;
}kfs_metadata_descriptor_t;



/* Right after the location of a kfs_slots_table_descriptor_t, a lot of 
 * structures kfs_slots_t should be stored. These data 
 * structures stores the actual location of the dictionary and the slot owner.
 * This is the on-disk representation of the slot_t data type. 
 */
typedef struct{ /* entry for the slots index. This structure helps to locate
                   slot data faster */
    uint64_t slot_id;     /* slot ID. Should be stored in sequence. */
    uint64_t slot_sino_owner;
    uint32_t slot_link_owner;
    uint32_t slot_flags;  /* the same flags than in slots.h in slot_t*/

    uint64_t slot_block;  /* block with this slot entries. Points to
                             a block with a kfs_table_header_t

                             kfs_slots_descriptor_header_t
                             structure */
}kfs_slot_t; /* size = 32 bytes */


/* kfs_slot_descriptor_t shows how is the slot dictionary stored on disk. 
 * The slot data may use one or more blocks. */
typedef struct{
    uint64_t slot_id;  /* slot ID */


    uint16_t total_blocks;    /* total of blocks for this slot */
    uint16_t data_block_seq;  /* which block of the total is this one */ 
    uint16_t total_elems;  /* total number of elements in the slot */ 
    uint16_t num_elems; /* number of elements for this slot in this block */
    uint16_t elems_seq; /* sequence number for the first element  
                           in this block */

    uint16_t data_offset;  /* offset in bytes to the first kv pair of this 
                              block. In that place, a kfs_slot_data_t 
                              should be found */

    uint16_t data_len; /* how many data bytes are used for this slot in this
                          block. We need this to calculate how many free 
                          space do we have */
    uint16_t spare_flags; /* extra room */
    uint64_t next_block; /* points to the next kfs_slots_descriptor_header 
                            block with kv pairs belonging to this slot_id. 
                            If 0, this block is a terminator for this slot, 
                            mean, all the data for this slot is stored in 
                            this block */
}kfs_slot_descriptor_t;


/* this is the real dictionary data. */
typedef struct{
    uint64_t hash_k;   /* hash for key */
    uint8_t rec_len;    /* this record size */
    uint8_t key_len;    /* key size */
    uint8_t value_type; /* value data type */
    uint8_t value_len;  /* value data length */
    uint16_t next_key;  /* offset to the next key. if 0, this is the 
                          terminator. if 0xffffffff, continues in other 
                          block */

    char kv_data[];    /* this field should be a blob with a 
                          zero terminated string, which is the key, followed
                          immediately with the 
                          value data */

}kfs_slot_data_t;




/* enough for slots. Now on to the real file system stuff. */ 



/* This is 
 * the edge representation in the block device */
typedef struct{
    uint64_t ed_link_id; /* link ID */
    uint64_t ed_sinode; /* which node is this edge pointing to */
    uint64_t ed_hash;   
    uint64_t ed_slot_id; /* slot ID */
    uint16_t ed_rec_len; /* this record total length ( multiple of 8) */
    uint8_t ed_flags;   /* flags for this edge */
    uint8_t ed_name_len;
    char ed_name[];
}kfs_edge_t;

typedef struct{
    uint64_t si_id;
    uint64_t si_slot_id; /* slot ID */
    time_t si_a_time;
    time_t si_c_time;
    time_t si_m_time;

    uint64_t si_data_len; /* file size */




typedef struct{
    kfs_edge_t **si_edges; /* array of edges of ptrs */

    uint64_t si_slot_id; /* slot ID */
    time_t si_a_time;
    time_t si_c_time;
    time_t si_m_time;

    uint64_t si_id; /* super inode unique ID */
    uint64_t si_data_len; /* file size */

#define KFS_INDIRECT_SINGLE      10
#define KFS_INDIRECT_DOUBLE      KFS_INDIRECT_SINGLE + 1 
#define KFS_INDIRECT_TRIPLE      KFS_INDIRECT_DOUBLE + 1
#define KFS_EXTENTS_NUM          KFS_INDIRECT_TRIPLE + 1

    /* for the extents we have a single array. This helps to save as much 
     * space as we can. Also, we have a variable boundary for separate data
     * from edges in si_edges_extents_start.
     *
     * So, the range for type of extents:
     * data:   element 0 to (si_edges_extents_start - 1)
     * edges:  element si_edges_extents_start to (KFS_INDIRECT_SINGLE - 1)
     * indirect single, double and triple have variable boundaries for 
     * edges and data */
    kfs_extent_t *si_extents[KFS_EXTENTS_NUM];

    uint32_t si_edges_extents_start;
    uint32_t si_edges_num;  /* total num of edges */
    

    /* cache flags may be used here */
    uint32_t si_flags;
    uint32_t si_gid;
    uint32_t si_uid;


    uint32_t si_mode; 
    uint32_t si_count;
}kfs_sinode_t; 




/* extents, slots, super inodes and graph entries should be cacheables */


/* Graph cache element */
typedef struct{
    kfs_edge_t **nd_edges;
    uint32_t nd_edges_num;
    uint8_t nd_visited[12]; /* 96 bits for graph searches */
    uint64_t nd_slot_id; 
    kfs_sinode_t *nd_sinode;
    time_t nd_lru;
}kfs_node_t;


typedef struct{
    uint64_t pc_path_hash;
    uint64_t pc_sinode_start;
    uint64_t pc_sinode_end;
    kfs_edge_t **pc_path;
    time_t pc_lru;
    uint32_t pc_count;
}kfs_path_cache_t;


typedef struct{
    uint64_t t_capacity;
    uint64_t t_in_use;
    kfs_extent_t *t_extent;
    kfs_cache_t *t_cache;
}kfs_table_t;


typedef struct{
    uint64_t sb_root_super_inode; /* any inode can be the root inode */

    /* extents cache */
    kfs_cache_t *sb_extents_cache;

    /* super inodes capacity, used inodes, cache, and extents */
    kfs_table_t sb_sinodes;

    /* slots capacity, used, cache and extents */
    kfs_table_t sb_slots;

    /* bit map capacity in blocks, taken and extents. Cache is not used. */
    kfs_table_t sb_blockmap;

    time_t sb_c_time, sb_m_time, sb_a_time; 

    uint64_t flags;

    /* block dev */
    int dev;
}kfs_sb_t;


typedef kfs_slot_t               slot_t;
typedef kfs_sinode_t             sinode_t;
typedef kfs_sb_t                 sb_t;
    
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

