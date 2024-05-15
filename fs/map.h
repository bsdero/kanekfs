#ifndef _MAP_H_
#define _MAP_H_

#ifdef USER_SPACE
 #include <stdint.h>
 #include <string.h>
#else
 #include <linux/types.h>
 #include <linux/string.h>
#endif


#define CLEARBIT                                   0
#define SETBIT                                     1

#ifndef min
#define min(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b;       \
})
#endif

/* set or clear numbits bits in the byte passed as argument, starting in 
 * the start bit. Will return the byte updated.
 */
unsigned char byte_set_bits( int start,
                             int numbits,
                             char byte,
                             int clear_or_set);

/* count how many contiguous bits has the byte passed as argument, 
 * starting in the startbit, counting max of numbits. clear_or_set 
 * specifies if we are looking for contiguous 0s or 1s. Will return
 * the number of contiguous bits */
int byte_count_bits( int start, int numbits, char byte, int clear_or_set);

/* find a gap of numbits, starting in start, in the byte passed as
 *  argument. Will return the offset of a found gap, *count will be
 *  updated with the number of contiguous bits found.
 *
 *  -1 if no gap was found, *count will be  = 0
 *  -2 if the end of byte was reached and we did not found enough
 *      numbits bits. *count will have the number of bits found */
int byte_find_gap( int start, int numbits, char byte, int *count);



/* set one bit in the blockmap pointed by bm, which has total_blocks bits. 
 * block_address is the bit to change and clear_or_set is a flag to set
 * or clear the bit. */
int bm_set_bit( unsigned char *bm,
                uint64_t total_blocks,
                uint64_t block_address, 
                int clear_or_set);

/* set a group of continuous bits in the blockmap pointed by bm, which has 
 * total_blocks bits. block_address is the start of the contiguous bits 
 * extent to update and clear_or_set is a flag to set or clear the bits. */
int bm_set_extent( unsigned char *bm,
                   uint64_t total_blocks,
                   uint64_t block_address,
                   uint64_t num_blocks_to_set,
                   int clear_or_set);


/* count how many contiguous 0 or 1 do we have in the blockmap pointed by 
 * bm, which has a size of total_blocks bits. The count starts in 
 * block_address, until num_blocks_to_find bits is reached. 
 * clear_or_set is a flag to count contiguous 1s or 0s. */
int bm_count( unsigned char *bm,
                   uint64_t total_blocks,
                   uint64_t block_address,
                   uint64_t num_blocks_to_count,
                   int clear_or_set,
                   uint64_t *count_result);

/* find a gap of contiguous zeroed bits in the blockmap pointed by 
 * bm, which has a size of total_blocks bits. We start to look for a gap 
 * in the address pointed by start, and will stop after count bits were
 * checked or until the total_blocks is reached. The size of the gap in 
 * bits is specified in gap_size. 
 * If the gap was found, return 0 and set found_adress to the bit offset
 * in the map where the gap is. 
 * If no gap was found, return -1.  */
int bm_find( unsigned char *bm,
             uint64_t total_blocks,
             uint64_t start,
             uint64_t count,
             uint64_t gap_size,
             uint64_t *found_address);


/* check if a group of contiguous enabled bits can grow into cleared bits
 * without change location in the blockmap pointed by bm, which has a size 
 * of total_blocks bits. start is the start address, which already includes
 * some 1s at the beginning. count is the number of bits that we need to 
 * resize. 
 * Will return: 
 * 0: If the extent can grow into trailing zeroes
 * -1 otherwise*/
int bm_extent_can_grow( unsigned char *bm, 
                          uint64_t total_blocks, 
                          uint64_t start, 
                          uint64_t count);

#endif

