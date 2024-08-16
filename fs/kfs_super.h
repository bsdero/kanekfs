#ifndef _KFS_SUPER_H_
#define _KFS_SUPER_H_

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




/* super block operations */
void kfs_sb_statfs();
void kfs_sb_sl_table_dump();
void kfs_sb_si_table_dump();
void kfs_sb_dump();
sinode_t kfs_sb_alloc_sinode(); /* alloc and fill in a super inode */
void kfs_sb_destroy_sinode( sinode_t *sinode); /* undo whatever done in
                                                kfs_sb_alloc_sinode */
slot_t kfs_sb_alloc_slot(); /* alloc and fill in a slot */
void kfs_sb_detroy_slot( slot_t *slot); /* undo whatever done in 
                                            kfs_sb_alloc_slot */
void kfs_sb_mount();
void kfs_sb_umount();
void kfs_sb_sync();
void kfs_sb_put_super();
void kfs_sb_get_super();
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

