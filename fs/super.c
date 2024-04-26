#include "kfs_ds.h"


sb_t __kfs_sb = { 0 };
                    

/* super block methods */
sb_t kfs_sb_create( uint64_t num_inodes, uint64_t num_slots){
    sb_t sb;

    sb.sb_c_time = sb.sb_m_time = sb.sb_a_time = time( NULL);
    sb.sb_root_super_inode = 0;

    sb.sb_extents_cache = NULL;
    sb.sb_sinodes = { 0 };
    sb.sb_slots = { 0 };
    sb.sb_blockmap = { 0 };
    sb.dev = -1;

    return sb;
}

/*
void kfs_sb_set_default( kfs_sb_t *kfs_sb);
void kfs_sb_statfs();
void kfs_sb_sl_table_dump();
void kfs_sb_si_table_dump(); 
*/
//sinode_t kfs_sb_alloc_sinode(); /* alloc and fill in a super inode */
//void kfs_sb_destroy_sinode( sinode_t *sinode); /* undo whatever done in
//                                                kfs_sb_alloc_sinode */
//slot_t kfs_sb_alloc_slot(); /* alloc and fill in a slot */
//void kfs_sb_detroy_slot( slot_t *slot); /* undo whatever done in 
//                                            kfs_sb_alloc_slot */
//void kfs_sb_sync();
//void kfs_sb_mkfs( char *file_name, uint64_t size_in_mbytes); /* mkfs */
//void kfs_sb_put_super();


