#include "kfs_mem.h"


extern sb_t __kfs_sb;
                    

/* super block methods */
/*
sb_t kfs_sb_create( uint64_t num_inodes, uint64_t num_slots){
    sb_t sb;

    sb.sb_c_time = sb.sb_m_time = sb.sb_a_time = time( NULL);
    sb.sb_root_super_inode = 0;

    sb.sb_extents_cache = NULL;
    sb.sb_sinodes.t_capacity = num_inodes;
    sb.sb_sinodes.t_in_use = 0;
    sb.sb_sinodes.t_extent = NULL;
    sb.sb_sinodes.t_cache = NULL;


    sb.sb_slots.t_capacity = num_slots;
    sb.sb_slots.t_in_use = 0;
    sb.sb_slots.t_extent = NULL;
    sb.sb_slots.t_cache = NULL;

    sb.dev = -1;

    return sb;
}

void kfs_sb_set_default( kfs_sb_t *kfs_sb){
    __kfs_sb = *kfs_sb;
}

void kfs_sb_statfs(){
    kfs_sb_t sb = __kfs_sb;
    char disk_type[32];

    printf("KanekFS, A graph file system\n");
    printf("   Root super inode id=%ul\n", sb.sb_root_super_inode );
    printf("   Super inodes capacity=%ul\n", sb.sb_sinodes.t_capacity );
    printf("   Super inodes in use=%ul\n", sb.sb_sinodes.t_in_use );
    printf("   Slots capacity=%ul\n", sb.sb_slots.t_capacity );
    printf("   Slots in use=%ul\n", sb.sb_slots.t_in_use );
}

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


