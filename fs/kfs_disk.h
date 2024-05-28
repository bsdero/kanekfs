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
#include "kfs_mem.h"


#define KFS_MAGIC                                  0x1ba7ba  
#define _1M                                        1048576
#define _1G                                        1073741824



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
 * terminators. A root block which also stores data and no have pointers, 
 * is called a "root leaf".
 *
 * But the root may reference data in extents of blocks, so it can act as 
 * an "index block". We can have also an index block with pointers to other 
 * index blocks too.
 *
 * In some cases, the root block may have the kfs_extent_header, followed by 
 * a single extent_entry and after it we can find data. In that case, the  
 * extent_entry may point to more data or index blocks/extents, and the first
 * data elements are located following the extent_entry. This was done for
 * save space, and we call those blocks as a "flower block". 
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
 *    we may store and how many are used in this block extent. The indexing 
 *    entries are stored in kfs_extent_entry_t data structures following the
 *    header. 
 * 4.-Additional data structures descripting data and possible table 
 *    headers are inmediately after the kfs_table_header_t. This depends on 
 *    the value of the "magic" field. 
 * 5.-If this is a "flower" extent, this is also a data root block. So we 
 *    set the flags KFS_ENTRIES_ROOT and KFS_ENTRIES_FLOWER. We have one 
 *    single extent_entry in the same block pointing to extents, but also 
 *    we have data following it. This is for use the disk space better.
 * 
 * So, we may have:
 * +-------+
 * | Header|
 * | data  |
 * | data  |
 * +-------+
 *  root leaf
 *
 * +--------+     +--------+     +--------+
 * | Header |     | Header |     | Header |
 * | ptr    |---->| ptr    |---->| data   |
 * | ptr    |--+  |        |     | data   |
 * +--------+  |  +--------+     +--------+
 *  root block |  index block     leaf block
 *             |
 *             |  +--------+
 *             +->| Header |
 *                | data   |
 *                +--------+
 *                leaf block
 *
 * +--------+     +--------+     +--------+
 * | Header |     | Header |     | Header |
 * | ptr    |---->| ptr    |---->| data   |
 * | data   |     |        |     | data   |
 * | data   |     |        |     | data   |
 * +--------+     +--------+     +--------+
 * flower block   index block     leaf block
 *
 *
 *
 * All the blocks/extents used for store tables data must have a table_header. 
 */
typedef struct{

/* those magics define what are we dealing with */
#define KFS_EXTENT_MAGIC                           0xaca771

    
/*#define KFS_SLOTS_ROOT_MAGIC                       0x0c01a7e*/
#define KFS_SLOTS_BITMAP_MAGIC                     0x1a77e
//#define KFS_SLOTS_TABLE_MAGIC                      0xdecaf
#define KFS_SLOTS_DATA_MAGIC                       0xc0ffee

#define KFS_SINODE_ROOT_MAGIC                      0xc001beb
#define KFS_SINODE_BITMAP_MAGIC                    0xbad50da
//#define KFS_SINODE_TABLE_MAGIC                     0xcafe
#define KFS_SINODE_DATA_MAGIC                      0xc0cac01a


    uint32_t eh_magic; /* table type */
    
    uint32_t eh_entries_in_use; /* number of used entries here */
    uint32_t eh_entries_capacity; /* max capacity of entries */

#define KFS_ENTRIES_ROOT                           0x000001    
#define KFS_ENTRIES_INDEX                          0x000002
#define KFS_ENTRIES_LEAF                           0x000004
#define KFS_ENTRIES_FLOWER                         0x000008
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
    uint16_t ee_block_size; /* number of blocks covered by extent */
    uint16_t ee_log_size; /* number of logical units */
    uint32_t ee_log_addr; /* offset in logical units */
}kfs_extent_entry_t;

typedef kfs_extent_entry_t kfs_extent_t;


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
 * For ease of locate for empty slots, a slot bitmap is provided.  
 * We can also update, read or remove the dictionary in the slot by using the
 * slot table, which holds details for each dictionary, number of elements, 
 * its extent, permissions and so on.
 * 
 * 
 *
 * STORAGE:
 * We organize the slots data in extents. So we have an extent for the slots
 * table, one extent for the bitmap and one extent per slot. 
 * In the slot table extent, we have the metadata descriptor, which descripts
 * the slots table. It maintains the address and size of the slots bitmap, 
 * how many free and used slots we have, slots capacity and more. 
 *
 * We also have the slots bitmap, for locate unused slots fast, the index
 * and one extent per dictionary/slot. 
 *
 * The metadata descriptor is a "root flower" block, with the field
 * extent_header.magic=KFS_SLOTS_TABLE_MAGIC. It has two extent entries 
 * pointing to both the bitmap extent and slots extent. After that we have the
 * the initial slots of the data structure, beginning with the slot 0. 
 *
 */


/* Right after the location of the corresponding kfs_extent_header_t a lot of 
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

    kfs_extent_entry_t slot_extent;  /* extent with this slot entries.*/
}kfs_slot_t; 


/* this is the real dictionary data. */
typedef struct{
    uint32_t hash_k;    /* hash for key */
    uint16_t rec_len;   /* this record size. Should be a multiple of 4 */
    uint8_t key_len;    /* key size */
    uint8_t value_type; /* value data type */
    uint16_t value_len;  /* value data length */
    char kv_data[];    /* this field should be a blob with a 
                          zero terminated string, which is the key, followed
                          immediately with the value data. So, this has a
                          size = value_len + key_len + 1. Then rec_len is 
                          rounded to the first immediate multiple of four. */

}kfs_slot_data_t;

/* enough for slots. Now on to the real file system stuff. */ 

/* Edges are represented also in an extent.
 */
typedef struct{
    uint64_t ed_link_id; /* link ID */
    uint64_t ed_slot_id; /* slot ID */
    
    uint64_t ed_sinode; /* which node is this edge pointing to */
    uint32_t ed_hash_name; 
    uint16_t ed_rec_len; /* this record total length ( multiple of 8) */
    uint8_t ed_flags;   /* flags for this edge */
    uint8_t ed_name_len;
    char ed_name[];
}kfs_edge_t;

/* this is the super inode structure in disk */
typedef struct{

    kfs_extent_entry_t edges;  /* extent with edges.*/
    kfs_extent_entry_t data;   /* extent with file data */
    time_t si_a_time;
    time_t si_c_time;
    time_t si_m_time;


    uint64_t si_slot_id; /* slot ID */
    uint64_t si_id; /* super inode unique ID */
    uint64_t si_data_len; /* file size */

    uint32_t si_edges_num;  /* total num of edges */

    /* cache flags may be used here */
    uint32_t si_flags;
    uint32_t si_gid;
    uint32_t si_uid;


    uint32_t si_mode; 
    uint32_t si_count;
}kfs_sinode_t; 


typedef struct{
    uint64_t capacity;
    uint64_t in_use;
    kfs_extent_t bitmap_extent; 
    kfs_extent_t table_extent;
}kfs_table_t;

typedef kfs_table_t kfs_slot_table_t;
typedef kfs_table_t kfs_si_table_t;
typedef kfs_table_t kfs_blockmap_t;

typedef struct{
    uint64_t sb_magic; 
    uint32_t sb_version;
    uint32_t sb_flags;
    uint64_t sb_root_super_inode; /* any inode can be the root inode */
    uint64_t sb_blocksize;
    
    /* super inodes capacity, used inodes, cache, and extents */
    kfs_si_table_t sb_si_table;

    /* slots capacity, used, cache and extents */
    kfs_slot_table_t sb_slot_table;

    /* bit map capacity in blocks, taken and extents. Cache is not used. */
    kfs_blockmap_t sb_blockmap;

    time_t sb_c_time;
    time_t sb_m_time;
    time_t sb_a_time; 


#define KFS_SB_SINODES_NUM_FIXED                   0x0001
#define KFS_SB_SLOTS_NUM_FIXED                     0x0002
#define KFS_SB_AUTO_DEFRAG                         0x0004
    
    uint64_t flags;

    /* block dev */
    int dev;
}kfs_superblock_t;

#endif

