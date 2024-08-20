#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include "kfs.h"
#include "kfs_io.h"
#include "kfs_page_cache.h"

sb_t __sb = { 0};


/* super block methods */
void ex_2_kfsex( kfs_extent_t *kex, extent_t *ex){
    kex->ee_block_addr = ex->ex_block_addr;
    kex->ee_block_size = ex->ex_block_size;
    kex->ee_log_size = ex->ex_log_size;
    kex->ee_log_addr = ex->ex_log_addr;
}

void kfsex_2_ex( extent_t *ex, kfs_extent_t *kex){
    ex->ex_block_addr = kex->ee_block_addr;
    ex->ex_block_size = kex->ee_block_size;
    ex->ex_log_size = kex->ee_log_size;
    ex->ex_log_addr = kex->ee_log_addr;
}


/*
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

*/


/* read super block from a memory block */
int kfs_load_superblock( kfs_superblock_t *kfs_sb, sb_t *sb){
    kfs_extent_t *kex;
    extent_t *ex;
     
    if( kfs_sb->sb_magic != KFS_MAGIC){
        TRACE_ERR("Not a KFS file system. Abort.");
        return( -1);
    }

    memset( sb, 0, sizeof( sb_t));

    sb->sb_root_super_inode = kfs_sb->sb_root_super_inode;
    sb->sb_flags = kfs_sb->sb_flags;
    sb->sb_c_time = kfs_sb->sb_c_time;
    sb->sb_m_time = kfs_sb->sb_m_time;
    sb->sb_a_time = kfs_sb->sb_a_time;
    sb->sb_magic = kfs_sb->sb_magic;
    sb->sb_si_table.capacity = kfs_sb->sb_si_table.capacity;
    sb->sb_si_table.in_use = kfs_sb->sb_si_table.in_use;
    kex = &kfs_sb->sb_si_table.table_extent;
    ex = &sb->sb_si_table.table_extent;
    kfsex_2_ex( ex, kex);
    kex = &kfs_sb->sb_si_table.bitmap_extent;
    ex = &sb->sb_si_table.bitmap_extent;
    kfsex_2_ex( ex, kex);

    sb->sb_slot_table.capacity = kfs_sb->sb_slot_table.capacity;
    sb->sb_slot_table.in_use = kfs_sb->sb_slot_table.in_use;
    kex = &kfs_sb->sb_slot_table.table_extent;
    ex = &sb->sb_slot_table.table_extent;
    kfsex_2_ex( ex, kex);
    kex = &kfs_sb->sb_slot_table.bitmap_extent;
    ex = &sb->sb_slot_table.bitmap_extent;
    kfsex_2_ex( ex, kex);


    sb->sb_blockmap.capacity = kfs_sb->sb_blockmap.capacity;
    sb->sb_blockmap.in_use = kfs_sb->sb_blockmap.in_use;
    kex = &kfs_sb->sb_blockmap.table_extent;
    ex = &sb->sb_blockmap.table_extent;
    kfsex_2_ex( ex, kex);
    kex = &kfs_sb->sb_blockmap.bitmap_extent;
    ex = &sb->sb_blockmap.bitmap_extent;
    kfsex_2_ex( ex, kex);

    return(0);
}


int kfs_mount( kfs_config_t *config, sb_t *sb){
    int rc = 0, fd;
    pgcache_t *pg_cache;
    pgcache_element_t *el;
    kfs_superblock_t *kfs_sb;
    time_t now;

    fd = kfs_open( config);
    if( fd < 0){
        TRACE_ERR("kfs_open() failed! abort.");
        rc = -1;
        goto exit1; 
    }

    pg_cache = kfs_pgcache_alloc( fd, config->cache_page_len);
    if( pg_cache == NULL){
        TRACE_ERR("Issues in kfs_pgcache_alloc()");
        rc = -1;
        goto exit1;
    }

    rc = kfs_pgcache_start_thread( pg_cache);
    if( rc != 0){
        TRACE_ERR("Issues in kfs_pgcache_start_thread()");
        goto exit0;
    }

    rc = kfs_pgcache_flags_wait( pg_cache, KFS_PGCACHE_ACTIVE, 10);
    if( rc != 0){
        TRACE_ERR("timeout waiting for KFS_PGCACHE_ACTIVE");
        goto exit0;
    }

    el = kfs_pgcache_element_map( pg_cache, 
                                  0, 1,  /* load block 0, one block */
                                  0, NULL); /*no flags, no on-evict callback */
    if( el == NULL){
        TRACE_ERR("Issues in kfs_pgcache_element_map()");
        goto exit0;
    }

    rc = kfs_pgcache_element_flags_wait( el, KFS_PGCACHE_ND_ACTIVE, 10);
    if( rc != 0){
        TRACE_ERR("timeout waiting for KFS_PGCACHE_ND_ACTIVE");
        goto exit0;
    }

    kfs_sb = (kfs_superblock_t *) el->ce_mem_ptr;
    rc = kfs_load_superblock( kfs_sb, sb);
    if( rc < 0){
        TRACE_ERR("Could not load superblock, abort");
        goto exit0;
    }

    sb->sb_bdev = fd;
    sb->sb_extents_cache = (void *) pg_cache; 
    sb->sb_cache_element = (void *) el;
    sb->sb_flags = kfs_sb->sb_flags |= KFS_IS_MOUNTED;

    now = time(NULL);
    sb->sb_m_time = sb->sb_a_time = now;
    kfs_sb->sb_m_time = kfs_sb->sb_a_time = now;

    kfs_pgcache_element_mark_dirty( el);
    __sb = *sb;

    goto exitOK;

exit0:
    kfs_pgcache_destroy( pg_cache);

exit1:
    close( fd);
exitOK:
    return( rc);
}


int kfs_active(){
    sb_t *sb = &__sb;
    pgcache_t *pgcache;
 
    if( sb->sb_magic != KFS_MAGIC){
        return( -1);
    }

    if( ( sb->sb_flags & KFS_IS_MOUNTED) == 0){
        return( -1);
    }

    pgcache = (pgcache_t *) sb->sb_extents_cache;

    if( (pgcache->ca_flags & KFS_PGCACHE_ACTIVE) == 0){
        return( -1);
    }
 
    return(0);
}

void kfs_superblock_display(){
    sb_t *sb = &__sb;
    char buff[240];
    extent_t *e;


    if( kfs_active() != 0 ){
        TRACE_ERR("Superblock is not active");
        return;
    }

    printf("SuperBlock\n");
    printf("KFS Flags: 0x%lx\n", sb->sb_flags);
    printf("KFS root super inode: %lu\n", sb->sb_root_super_inode);

    strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime( &sb->sb_c_time));
    printf("KFS Ctime: %s\n", buff); 

    strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime( &sb->sb_a_time));    
    printf("KFS Atime: %s\n", buff); 

    strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime( &sb->sb_m_time));
    printf("KFS Mtime: %s\n", buff); 

    printf("SUPER INODES:\n");
    printf("    NumberOfSuperInodes: %lu\n", sb->sb_si_table.capacity );
    printf("    SuperInodesInUse: %lu\n", sb->sb_si_table.in_use ); 
    printf("    SuperInodesTableAddress: %lu\n",  
                sb->sb_si_table.table_extent.ex_block_addr );
    printf("    SuperInodesTableBlocksNum: %u\n",  
                sb->sb_si_table.table_extent.ex_block_size );
    printf("    SuperInodesMapAddress: %lu\n",  
                sb->sb_si_table.bitmap_extent.ex_block_addr );
    printf("    SuperInodesMapBlocksNum: %u\n",  
                sb->sb_si_table.bitmap_extent.ex_block_size );

    printf("SLOTS:\n");
    printf("    NumberOfSlots: %lu\n", sb->sb_slot_table.capacity );
    printf("    SlotsInUse: %lu\n", sb->sb_slot_table.in_use ); 
    printf("    SlotsTableAddress: %lu\n",  
                sb->sb_slot_table.table_extent.ex_block_addr );
    printf("    SlotsTableBlocksNum: %u\n",  
                sb->sb_slot_table.table_extent.ex_block_size );
    printf("    SlotsMapAddress: %lu\n",  
                sb->sb_slot_table.bitmap_extent.ex_block_addr );
    printf("    SlotsMapBlocksNum: %u\n",  
                sb->sb_slot_table.bitmap_extent.ex_block_size );

    printf("KFS Bitmap:\n");
    printf("    KFSBlockMapAddress: %lu\n",  
                sb->sb_blockmap.bitmap_extent.ex_block_addr );
    printf("    KFSBlockMapBlocksNum: %u\n",  
                sb->sb_blockmap.bitmap_extent.ex_block_size );
        
    printf("KFS BlocksRange:\n");
    printf("    SuperBlock: [0]\n");

    e = &sb->sb_si_table.table_extent;
    printf("    SuperInodesTable: [%lu-%lu]\n",
           e->ex_block_addr,
           e->ex_block_addr + e->ex_block_size - 1);

    e = &sb->sb_slot_table.table_extent;
    printf("    SlotsTable: [%lu-%lu]\n",
           e->ex_block_addr,
           e->ex_block_addr + e->ex_block_size - 1);
               
    e = &sb->sb_si_table.bitmap_extent;
    printf("    SuperInodesMap: [%lu-%lu]\n",
           e->ex_block_addr,
           e->ex_block_addr + e->ex_block_size - 1);

    e = &sb->sb_slot_table.bitmap_extent;
    printf("    SlotsMap: [%lu-%lu]\n",
           e->ex_block_addr,
           e->ex_block_addr + e->ex_block_size - 1);

    e = &sb->sb_blockmap.bitmap_extent;
    printf("    KFSBlockMap: [%lu-%lu]\n",
           e->ex_block_addr,
           e->ex_block_addr + e->ex_block_size - 1);

}


int kfs_umount(){
    sb_t *sb = &__sb;
    int rc = 0;
    pgcache_element_t *el;
    pgcache_t *pgcache;

    if( kfs_active() != 0 ){
        TRACE_ERR("Superblock is not active");
        rc = -1;
    }

    pgcache = (pgcache_t *) sb->sb_extents_cache;
    el = ( pgcache_element_t *) sb->sb_cache_element; 
 
    sb->sb_flags = sb->sb_flags &~ KFS_IS_MOUNTED;
    kfs_superblock_update();

    /* wait for the super block to be updated */
    rc = kfs_pgcache_element_flags_wait( el, KFS_PGCACHE_ND_CLEAN, 10);
    if( rc != 0){
        TRACE_ERR("timeout waiting for KFS_PGCACHE_ND_CLEAN");
    }

    kfs_pgcache_destroy( pgcache);
    close( sb->sb_bdev);
    memset( &__sb, 0, sizeof( sb_t ));

    return(rc);
}

    

int kfs_superblock_update(){
    pgcache_element_t *el;
    kfs_superblock_t *ksb;
    kfs_extent_t *kex;
    extent_t *ex;
    time_t now;
    sb_t *sb = &__sb;

    if( kfs_active() != 0 ){
        TRACE_ERR("Superblock is not active");
        return( -1);
    }
   
    el = ( pgcache_element_t *) sb->sb_cache_element; 
    ksb = (kfs_superblock_t *) el->ce_mem_ptr;

    if( ksb->sb_magic != KFS_MAGIC){
        TRACE_ERR("Not a KFS file system. Abort.");
        return( -1);
    }

    now = time( NULL);

    ksb->sb_m_time = sb->sb_m_time = now; 
    ksb->sb_a_time = sb->sb_a_time = now; 

    ksb->sb_root_super_inode = sb->sb_root_super_inode;
    ksb->sb_flags = sb->sb_flags;

    ksb->sb_si_table.capacity = sb->sb_si_table.capacity;
    ksb->sb_si_table.in_use = sb->sb_si_table.in_use;
    kex = &ksb->sb_si_table.table_extent;
    ex = &sb->sb_si_table.table_extent;
    ex_2_kfsex( kex, ex);
    kex = &ksb->sb_si_table.bitmap_extent;
    ex = &sb->sb_si_table.bitmap_extent;
    ex_2_kfsex( kex, ex);

    ksb->sb_slot_table.capacity = sb->sb_slot_table.capacity;
    ksb->sb_slot_table.in_use = sb->sb_slot_table.in_use;
    kex = &ksb->sb_slot_table.table_extent;
    ex = &sb->sb_slot_table.table_extent;
    ex_2_kfsex( kex, ex);
    kex = &ksb->sb_slot_table.bitmap_extent;
    ex = &sb->sb_slot_table.bitmap_extent;
    ex_2_kfsex( kex, ex);


    ksb->sb_blockmap.capacity = sb->sb_blockmap.capacity;
    ksb->sb_blockmap.in_use = sb->sb_blockmap.in_use;
    kex = &ksb->sb_blockmap.table_extent;
    ex = &sb->sb_blockmap.table_extent;
    ex_2_kfsex( kex, ex);
    kex = &ksb->sb_blockmap.bitmap_extent;
    ex = &sb->sb_blockmap.bitmap_extent;
    ex_2_kfsex( kex, ex);

    kfs_pgcache_element_mark_dirty( el);
 
    return(0);
}


int kfs_slot_reserve( uint64_t *slot_id ){
    char *p;
    unsigned char *bitmap;
    uint64_t slotmap_blocks_num, slotmap_block, total_slots;
    uint64_t slot_block, slots_per_block, slot_offset; 
    kfs_extent_header_t *ex_header;
    int rc;
    sb_t *sb = &__sb;
    kfs_slot_t *kslot;


    if( kfs_active() != 0 ){
        TRACE_ERR("Superblock is not active");
        return( -1);
    }
    
    /* check if we have enough room for an extra slot */
    if( sb->sb_slot_table.in_use > sb->sb_slot_table.capacity ){
        TRACE_ERR("No more slots available");
        return( -1);
    }

    slotmap_block = sb->sb_slot_table.bitmap_extent.ex_block_addr;
    slotmap_blocks_num = sb->sb_slot_table.bitmap_extent.ex_block_size;

    /* alloc blocks */
    p = pages_alloc( slotmap_blocks_num);
    if( p == NULL){
        TRACE_ERR("Memory for SlotMap could not be reserved");
        return( -1);
    }

    /* read extent */
    rc = extent_read( sb->sb_bdev, p, slotmap_block, slotmap_blocks_num);
    if( rc == 0){
        TRACE_ERR("Could not read all, rc=%d", rc);
        return( -1);
    }

    ex_header = (kfs_extent_header_t *) p;

    /* verify this is a slot map block */
    if( ex_header->eh_magic != KFS_SLOTS_BITMAP_MAGIC){
        TRACE_ERR("Not a slot map block");
        return( -1);
    }

    /* double check the map has enough room for an extra slot, but
     * now we check in the map extent */
    if( ex_header->eh_entries_in_use > ex_header->eh_entries_capacity ){
        TRACE_ERR("No more slots available");
        return( -1);
    }

    total_slots = sb->sb_slot_table.capacity; 
/*    ex_header->eh_entries_in_use = 0;
    ex_header->eh_entries_capacity = bc->out_slots_num;
    ex_header->eh_flags = KFS_ENTRIES_ROOT|KFS_ENTRIES_LEAF;*/
 
    bitmap = (unsigned char *) p + sizeof( kfs_extent_header_t);
    rc = bm_find( bitmap, 
                  total_slots, 
                  0,
                  total_slots,
                  1,
                  slot_id);


    if( rc != 0){
        TRACE_ERR("A slot could not be found in the map");
        return( -1);
    }

    /* mark the slot as busy */
    rc = bm_set_bit( bitmap, total_slots, *slot_id, 1);

    /* update superblock data */
    sb->sb_slot_table.in_use++;

    /* update slot map header */
    ex_header->eh_entries_in_use = sb->sb_slot_table.in_use;

    /* write extent */
    rc = extent_write( sb->sb_bdev, p, slotmap_block, slotmap_blocks_num);
    if( rc == 0){
        TRACE_ERR("Could not write, rc=%d", rc);
        return( -1);
    }

    free( p);


    /* next, update slot and mark it as used */
    slots_per_block = (KFS_BLOCKSIZE - sizeof( kfs_extent_header_t)) / 
                      sizeof( kfs_slot_t);

    
    slot_offset = 0;
    slot_block = ( *slot_id * slots_per_block);
    if( slot_block == 0){
        slot_offset = sizeof( kfs_extent_t);
    }

    if( slot_block > sb->sb_si_table.table_extent.ex_block_size){
        TRACE_ERR( "slot block is outside the slots table. ");
        TRACE_ERR( "slot=[%lu, 0x%lx], slot_block=[%lu, 0x%lx]", 
                   *slot_id, *slot_id,
                    slot_block, slot_block);
        return( -1);
    }

    slot_block += sb->sb_si_table.table_extent.ex_block_addr;

    p = pages_alloc( 1);
    if( p == NULL){
        TRACE_ERR("Memory for SlotMap could not be reserved");
        return( -1);
    }


    rc = block_read( sb->sb_bdev, p, slot_block);
    if( rc == 0){
        TRACE_ERR("Could not read page, rc=%d", rc);
        return( -1);
    }

    kslot = ( kfs_slot_t *) p + (( *slot_id % KFS_BLOCKSIZE) + slot_offset);
    if( kslot->slot_id == *slot_id){
        TRACE_ERR("Could not find slot. ");
        TRACE_ERR("slot_id=[%lu, 0x%lx], got: [%lu, 0x%lx]",
                  *slot_id, *slot_id,
                   kslot->slot_id, kslot->slot_id);

        return( -1);
    }


    kslot->slot_flags |= SLOT_IN_USE;

    rc = block_write( sb->sb_bdev, p, slot_block);
    if( rc == 0){
        TRACE_ERR("Could not write page, rc=%d", rc);
        return( -1);
    }

    /* and update the slot index */
    rc = block_read( sb->sb_bdev, p, 
                     sb->sb_si_table.table_extent.ex_block_addr);
    if( rc == 0){
        TRACE_ERR("Could not read page, rc=%d", rc);
        return( -1);
    }

    ex_header = (kfs_extent_header_t *) p;

    /* verify this is a slot table block */
    if( ex_header->eh_magic != KFS_SLOTS_TABLE_MAGIC){
        TRACE_ERR("Not a slot table block");
        return( -1);
    }

    ex_header->eh_entries_in_use = sb->sb_slot_table.in_use;
    /* and update the slot index */
    rc = block_write( sb->sb_bdev, p, 
                      sb->sb_si_table.table_extent.ex_block_addr);
    if( rc == 0){
        TRACE_ERR("Could not write page, rc=%d", rc);
        return( -1);
    }

    free(p);
    return(0);
}

int kfs_slot_remove(uint64_t slot_id){
    char *p;
    unsigned char *bitmap;
    uint64_t slotmap_blocks_num, slotmap_block, total_slots;
    uint64_t slot_block, slots_per_block, slot_offset; 
    kfs_extent_header_t *ex_header;
    int rc;
    sb_t *sb = &__sb;
    kfs_slot_t *kslot;


    if( kfs_active() != 0 ){
        TRACE_ERR("Superblock is not active");
        return( -1);
    }
    
    /* check if we have enough room for an extra slot */
    if( slot_id > sb->sb_slot_table.capacity ){
        TRACE_ERR( "Wrong slot id beyond capacity. ");
        TRACE_ERR( "slot_id=[%lu, 0x%lx]",
                    slot_id, slot_id);
        return( -1);
    }

    slotmap_block = sb->sb_slot_table.bitmap_extent.ex_block_addr;
    slotmap_blocks_num = sb->sb_slot_table.bitmap_extent.ex_block_size;

    /* alloc blocks */
    p = pages_alloc( slotmap_blocks_num);
    if( p == NULL){
        TRACE_ERR("Memory for SlotMap could not be reserved");
        return( -1);
    }

    /* read extent */
    rc = extent_read( sb->sb_bdev, p, slotmap_block, slotmap_blocks_num);
    if( rc == 0){
        TRACE_ERR("Could not read all, rc=%d", rc);
        return( -1);
    }

    ex_header = (kfs_extent_header_t *) p;

    /* verify this is a slot map block */
    if( ex_header->eh_magic != KFS_SLOTS_BITMAP_MAGIC){
        TRACE_ERR("Not a slot map block");
        return( -1);
    }

    total_slots = sb->sb_slot_table.capacity; 
/*    ex_header->eh_entries_in_use = 0;
    ex_header->eh_entries_capacity = bc->out_slots_num;
    ex_header->eh_flags = KFS_ENTRIES_ROOT|KFS_ENTRIES_LEAF;*/
 
    bitmap = (unsigned char *) p + sizeof( kfs_extent_header_t);

    /* clean the slot  */
    rc = bm_set_bit( bitmap, total_slots, slot_id, 0);

    /* update superblock data */
    sb->sb_slot_table.in_use--;

    /* update slot map header */
    ex_header->eh_entries_in_use = sb->sb_slot_table.in_use;

    /* write extent */
    rc = extent_write( sb->sb_bdev, p, slotmap_block, slotmap_blocks_num);
    if( rc == 0){
        TRACE_ERR("Could not write, rc=%d", rc);
        return( -1);
    }

    free( p);


    /* next, update slot and mark it as used */
    slots_per_block = (KFS_BLOCKSIZE - sizeof( kfs_extent_header_t)) / 
                      sizeof( kfs_slot_t);

    
    slot_offset = 0;
    slot_block = ( slot_id * slots_per_block);
    if( slot_block == 0){
        slot_offset = sizeof( kfs_extent_t);
    }

    if( slot_block > sb->sb_si_table.table_extent.ex_block_size){
        TRACE_ERR( "slot block is outside the slots table. ");
        TRACE_ERR( "slot=[%lu, 0x%lx], slot_block=[%lu, 0x%lx]", 
                   slot_id, slot_id,
                   slot_block, slot_block);
        return( -1);
    }

    slot_block += sb->sb_si_table.table_extent.ex_block_addr;

    p = pages_alloc( 1);
    if( p == NULL){
        TRACE_ERR("Memory for SlotMap could not be reserved");
        return( -1);
    }


    rc = block_read( sb->sb_bdev, p, slot_block);
    if( rc == 0){
        TRACE_ERR("Could not read page, rc=%d", rc);
        return( -1);
    }

    kslot = ( kfs_slot_t *) p + (( slot_id % KFS_BLOCKSIZE) + slot_offset);
    if( kslot->slot_id == slot_id){
        TRACE_ERR("Could not find slot.");
        TRACE_ERR("slot_id=[%lu, 0x%lx], got: [%lu, 0x%lx]",
                   slot_id, slot_id,
                   kslot->slot_id, kslot->slot_id);

        return( -1);
    }


    kslot->slot_flags = 0;

    rc = block_write( sb->sb_bdev, p, slot_block);
    if( rc == 0){
        TRACE_ERR("Could not write page, rc=%d", rc);
        return( -1);
    }

    /* and update the slot index */
    rc = block_read( sb->sb_bdev, p, 
                     sb->sb_si_table.table_extent.ex_block_addr);
    if( rc == 0){
        TRACE_ERR("Could not read page, rc=%d", rc);
        return( -1);
    }

    ex_header = (kfs_extent_header_t *) p;

    /* verify this is a slot table block */
    if( ex_header->eh_magic != KFS_SLOTS_TABLE_MAGIC){
        TRACE_ERR("Not a slot table block");
        return( -1);
    }

    ex_header->eh_entries_in_use = sb->sb_slot_table.in_use;
    /* and update the slot index */
    rc = block_write( sb->sb_bdev, p, 
                      sb->sb_si_table.table_extent.ex_block_addr);
    if( rc == 0){
        TRACE_ERR("Could not write page, rc=%d", rc);
        return( -1);
    }

    free(p);
    return(0);

}


