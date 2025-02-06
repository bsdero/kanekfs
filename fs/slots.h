
/* For the slots the next operations can be done: 
 *
 * reserve         -looks for an empty slot and open it
 * get             -open an existing slot
 * put             -close an existing slot
 * evict           -evict an existing slot
 *
 * once the slots are open, the dict can be read or written. 
 * The operations when the slots are open:
 * update          -dict get updated 
 * The operations are exclusive, and the slot are locked. 
 *
 * For the cache: 
 * reserve:        -looks for an empty slot in the slot map, update and 
 *                  dirtymark
 *                 -load the slot index block into the page cache
 *                 -update superblock with number of slots
 *
 * get             -load the slot index block of the requested slot id into
 *                  the page cache
 *                 -load the page cache of the dict extent, load the dict
 *                  
 * update and put  -if the dict fits into the dict extent, just update and 
 *                  mark the extent as dirty.
 *                  If it does not, reserve an extent in the block map, load 
 *                  into the page cache, update, mark as dirty. 
 *                  Then update the slot in the cached index extent, mark as 
 *                  dirty 
 *                  blank the old dict extent, mark as dirty and free the
 *                  extent in the map
 *                  
 * evict           -update dict index, blank dict extent, mark both as dirty
 *                 -update number of available slots in superblock, and put it
 *                 -update slot map and block map
 *
 *
 */

