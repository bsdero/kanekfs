#include "kfs.h"
#include "eio.h"
#include "page_cache.h"

#include "slots.h"

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


