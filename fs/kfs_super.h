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


#define kfs_get_sb()                     ( &__sb)

int kfs_verify( char *filename, int verbose, int extra_verification);
int kfs_mount( kfs_config_t *config);
int kfs_active();
void kfs_superblock_display();
int kfs_umount();
int kfs_superblock_update();



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

