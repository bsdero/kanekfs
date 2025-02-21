#ifndef _SLOTS_H_
#define _SLOTS_H_

#include "kfs_super.h"




/* operations with slots */
slot_t *slot_alloc( uint64_t inode, 
                    uint16_t edge, 
                    uint16_t flags);

int slot_peek( uint64_t slot_id, dict_d *dict); /* open, read, close a slot */

int slot_peek_owner( uint64_t slot_id, 
                     uint64_t *inode,
                     uint16_t *edge,
                     uint16_t *flags);

slot_t *slot_open( uint64_t slot_id); /* open and read a slot */
int slot_set_owner( slot_t *s, 
                    uint64_t inode, 
                    uint16_t edge, 
                    uint16_t flags);
int slot_set_dict( slot_t *s, dict_t d); /* set new key-values to slot */
int slot_dump( slot_t *slot); /* dump slot */
int slot_destroy( slot_t *slot);
int slot_write( slot_t *slot); /* flush the slot to disk */
int slot_close( slot_t *slot); /* close slot */
int slot_evict( uint64_t slot_id); /* evict a slot from storage*/
/**************************************************************************
 * KEY-VALUES SLOTS BEHAVIOR
 * If only read key values and ownership is required, the peek functions are 
 * enough. No locking, or strange stuff.  
 *
 * If updates are needed, the access is exclusive, so while the slot is open
 * nobody else can update it, although reads are allowed for all.
 *
 * In order to get exclusive access to an slot, it should be open first,
 * updated and then closed. 
 *
 *
 * alloc           -looks for an empty slot and open it, is empty and need
 *                  key-values and owner.
 * open            -open an existing slot, read it.
 * peek            -open, reads and close, fill in the dict.
 * peek_owner      -open, reads and close, fill in the dict. 
 * set_owner       -set inode and edge owner, also ownership flags. Only 
 *                  for open slots.
 * set_dict        -Updates the key-values. Only for open slots.
 * put             -close an open slot, free slot memory.
 * evict           -evict an existing slot from storage.
 * dump            -Dump an already open slot
 *
 * once the slots are open, the dict can be read or written. 
 * The operations when the slots are open:
 *
 * For the cache: 
 * alloc:          -looks for an empty slot in the slot map, update and 
 *                  dirtymark
 *                 -load the slot index block into the page cache
 *                 -update superblock with number of slots
 *                 -fill in an empty slot, and lock. Return slot.
 *
 * open, peek      -load the slot index block of the requested slot id into
 *                  the page cache
 *                 -load the corresponding dict extent into cache
 *                 -read dictionary
 *                 -fill in an empty slot with data. 
 *                 -If open, lock slot and return slot.
 *                 -If peek, fill in the dict, free slot and return dict
 *                  
 * set dict, set owner
 *                 -The steps apply fo update owner. 
 *                 -if the dict fits into the dict extent, just update and 
 *                  mark the extent as dirty.
 *                 -update the index, mark as dirty also. 
 *
 *                 -If we are updating a dict and it does not fit in the
 *                  existing slot extent, 
 *                   -load block map
 *                   -reserve a bigger extent in the block map and dirty mark
 *                   -build a new extent in memory, fill in with key values,
 *                    map with a corresponding extent in disk, mark as dirty
 *                   -update the index extent with the key-values extent, mark
 *                    as dirty
 *                   -blank the old dict extent, mark as dirty and free the
 *                    extent in the map
 *                  
 * evict           -update dict index, blank dict extent, mark both as dirty
 *                 -update number of available slots in superblock, and put it
 *                 -update slot map and block map
 *
 *
 */

#endif
