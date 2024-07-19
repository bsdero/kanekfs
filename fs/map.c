#include "map.h"
#ifndef USER_SPACE
extern unsigned int trace_level;
extern unsigned int trace_mask;
#endif

/* set or clear numbits bits in the byte passed as argument, starting in 
 * the start bit. Return the updated byte.
 */
unsigned char byte_set_bits( int start,
                             int numbits,
                             char byte,
                             int clear_or_set){
    int to = start + numbits; /* which bit to stop */
    uint8_t mask = 0x00; 

    if( to > 8){
        to = 8;
    }

    do{
        mask |= (0x01 << start++); 
    }while( start < to);

    /* if  turn the bits on */
    if( clear_or_set == SETBIT){
        byte |= mask; /* use the or */
    }else{
        byte &= ~mask;  /* turn off the bits */
    }

    return( byte); 
}


/* get bit status in a byte */
int byte_get_bit( unsigned char byte, int bit){
    int rc;

    rc = byte & (1 << bit);
    return( ( rc == 0) ? 0 : 1);
}


/* count how many contiguous bits has the byte passed as argument,
 * starting in the startbit, counting max of numbits. clear_or_set
 * specifies if we are looking for contiguous 0s or 1s. Will return
 * the number of contiguous bits */
int byte_count_bits( int start, int numbits, char byte, int clear_or_set){
    int to = start + numbits; /* which bit to stop */
    uint8_t mask = 0x00;
    int count = 0;

    if( to > 8){
        to = 8;
    }

    /* turn on the required bits in the mask */
    do{
        mask = 0x01 << start;
        if( mask & byte){
            if( clear_or_set == SETBIT){
                count++;
            }else{
                return( count);
            }
        }else{
            if( clear_or_set != SETBIT){
                count++;
            }else{
                return( count);
            }
        }
        start++;
    }while( start < to);
    return( count);
}


/* find a gap of numbits, starting in start, in the byte passed as
 *  argument. Will return the offset of a found gap, *count will be
 *  updated with the number of contiguous bits found. 
 *
 *  Will return: 
 *   0 or greater: offset of a found gap
 *  -1 if no gap was found, *count will be  = 0
 *  -2 if the end of byte was reached and we did not found enough 
 *      numbits bits. *count will have the number of bits found */
int byte_find_gap( int start, int numbits, char byte, int *count){
    uint8_t mask = 0x00;
    int found_gap = 0; 
    int gap_index = start;

    *count = 0;

    while( start < 8){
        mask = (0x01 << start++);
        if( mask & byte){
            found_gap = 0;
            *count = 0;
            gap_index = start;
            continue;
        }

        found_gap = 1;
        
        (*count)++;
        if( *count == numbits){
            /* found a gap */
            return( gap_index);
        }
    }

    if( found_gap == 0){
        return(-1); /* did not found a gap */
    }

    return( -2);
}




/* set one bit in the bitmap pointed by bm, which has total_bits bits. 
 * bit_address is the bit to change and clear_or_set is a flag to set
 * or clear the bit. */
int bm_set_bit( unsigned char *bm,
                uint64_t total_bits,
                uint64_t bit_address, 
                int clear_or_set){

    /* do the math for know which byte in the map should we update */
    uint64_t location_in_map;
    uint8_t bit_to_change;
    uint8_t byte; 

    if( bit_address >= total_bits){
        return( -1);
    }

    /* get byte index in the map */
    location_in_map = bit_address / 8;

    /* get which bit to update */
    bit_to_change = bit_address % 8;

    /* get the byte from the map */ 
    byte = bm[location_in_map];

    /* update just one bit in the byte */
    byte = byte_set_bits( bit_to_change, 1, byte, clear_or_set);
    
    /* store the byte in the map */
    bm[location_in_map] = byte; 
    return(0);
}

/* get bit status in a bitmap */
int bm_get_bit( unsigned char *bm, 
                uint64_t total_bits, 
                uint64_t bit_address){

    /* do the math for know which byte in the map should we update */
    uint64_t location_in_map;
    uint8_t bit_to_read;
    uint8_t byte; 
    int rc;

    if( bit_address >= total_bits){
        return( -1);
    }

    /* get byte index in the map */
    location_in_map = bit_address / 8;

    /* get which bit to read */
    bit_to_read = bit_address % 8;

    /* get the byte from the map */ 
    byte = bm[location_in_map];

    /* get bit status */
    rc = byte_get_bit( byte, (int) bit_to_read);
    
    return(rc);
}

/* set a group of continuous bits in the bitmap pointed by bm, which has 
 * total_bits bits. bit_address is the start of the contiguous bits 
 * extent to update and clear_or_set is a flag to set or clear the bits. */
int bm_set_extent( unsigned char *bm,
                   uint64_t total_bits,
                   uint64_t bit_address,
                   uint64_t num_bits_to_set,
                   int clear_or_set){

    uint64_t location_in_map;
    uint8_t byte, bytes_in_map;
    uint64_t pending_bits = 0;
    uint64_t bits_to_mark_at_start;
    uint64_t bytes_to_mark;
    uint64_t extra_bits_to_mark;
    int aligned_to_byte;

    if( (bit_address > total_bits) ||
        (bit_address + num_bits_to_set) > total_bits) {
        return( -1);
    }

    /* if only need to mark one bit, call bm_set_bit */
    if( num_bits_to_set == 1){
        return( bm_set_bit( bm, total_bits, bit_address, clear_or_set));
    }


    location_in_map = bit_address / 8;
    pending_bits = num_bits_to_set;
    
    /* get if the bit address is byte aligned */ 
    aligned_to_byte = bit_address & 0x07;

    /* not aligned, process the first byte accordingly */
    if( aligned_to_byte != 0 ){
	     
	/* get the first byte */
        byte = bm[location_in_map];

	/* calc hoy many bits we need to update for the first byte. If 
	 * num_bit_to_mark is biggest than 7 and the min
	 * number of bits is 7, we don't care because the value will be 
	 * fixed in the byte_set_bits() function.  */
        
        if( (aligned_to_byte + num_bits_to_set) < 8){
            bits_to_mark_at_start = num_bits_to_set; 
        }else{
            bits_to_mark_at_start = 8 - aligned_to_byte;
        } 

        /* set or clear the bits */
        byte = byte_set_bits( aligned_to_byte, 
                              bits_to_mark_at_start, 
                              byte, 
                              clear_or_set);

	/* store in the map */
        bm[location_in_map] = byte;

	/* now we have lesser pending bits */
        pending_bits -= bits_to_mark_at_start;

	/* process the next byte */
        location_in_map++;

	/* possibly we are done. If that's the case, just quit */
        if( pending_bits == 0){
            return(0);
        }
    }

    /* now get how many bytes we need to update */
    bytes_to_mark = pending_bits / 8;

    /* if we got 1 or more full bytes */
    if( bytes_to_mark >= 1){
        if( clear_or_set == 0){
            bytes_in_map = 0x00;
        }else{
            bytes_in_map = 0xff;
        }
	/* update the bytes */
        memset( &bm[location_in_map], bytes_in_map, bytes_to_mark);
        location_in_map += bytes_to_mark;
        pending_bits -= (bytes_to_mark * 8);
        if( pending_bits == 0){
            return(0);
        }
    }


    /* if we re here, that means we have lesser than 8 bits to update in 
     * the bitmap. */
    extra_bits_to_mark = pending_bits % 8;

    if( extra_bits_to_mark > 1){
        byte = bm[location_in_map];
	byte = byte_set_bits( 0, extra_bits_to_mark, byte, clear_or_set);
        bm[location_in_map] = byte;
        pending_bits -= extra_bits_to_mark;
        if( pending_bits == 0){
            return(0);
        }
    }
    return(-2);
}

/* count how many contiguous 0 or 1 do we have in the bitmap pointed by
 * bm, which has a size of total_bits bits. The count starts in
 * bit_address, until num_bits_to_count bits is reached.
 * clear_or_set is a flag to count contiguous 1s or 0s. 
 * Return: 
 *        0 always, except error. 
 */

int bm_count( unsigned char *bm,
                   uint64_t total_bits, 
                   uint64_t bit_address,
                   uint64_t num_bits_to_count, 
                   int clear_or_set, 
                   uint64_t *count_result){

    uint64_t location_in_map;
    uint8_t byte;
    uint64_t bits_count = 0; 
    int64_t pending_bits = num_bits_to_count;
    int expected_bits = 0;
    int aligned_to_byte;

    *count_result = 0;


    if( (bit_address >= total_bits) ||
        (bit_address + num_bits_to_count) > total_bits) {
        return( -1);
    }
 
    bits_count = 0;
    location_in_map = bit_address / 8;

    /* get if the bit address is byte aligned */
    aligned_to_byte = bit_address & 0x07;

    /* bit not aligned to byte start */ 
    if( aligned_to_byte != 0){
        /* process first byte */
        byte = bm[location_in_map++];


        if( (aligned_to_byte + num_bits_to_count) >= 8){
            expected_bits = 8 - aligned_to_byte;
        }else{
            expected_bits = num_bits_to_count;
        }

        bits_count = byte_count_bits( (int) aligned_to_byte, 
                                      (int) expected_bits, 
                                      byte, 
                                      clear_or_set);


        pending_bits -= expected_bits;

        if( pending_bits <= 0 || bits_count < expected_bits){
            *count_result = bits_count;
            return( 0);
        }
    }


    while( bits_count < num_bits_to_count){
        byte = bm[location_in_map++];

        if( byte != 0xff && byte != 0x00 ){
            bits_count += byte_count_bits( (int) 0,
                                           pending_bits,
                                           byte,
                                           clear_or_set);

            *count_result = bits_count;

            return(0);
        }else{
            if( ( clear_or_set == 1 && byte == 0xff) ||
                ( clear_or_set == 0 && byte == 0x00)){


                if( pending_bits <= 8){
                    *count_result = num_bits_to_count;
                    return( 0);
                }
                bits_count += 8;
                pending_bits -= 8;
            }else{
                *count_result = bits_count;
                return( 0);
            }
        }
    }

    return( -2);
}

/* find a gap of contiguous zeroed bits in the bitmap pointed by
 * bm, which has a size of total_bits bits. We start to look for a gap
 * in the address pointed by start, and will stop after count bits were
 * checked or until the total_bits is reached. The size of the gap in
 * bits is specified in gap_size.
 * If the gap was found, return 0 and set found_adress to the bit offset
 * in the map where the gap is.
 * If no gap was found, return -1.  */
int bm_find( unsigned char *bm,
             uint64_t total_bits,
             uint64_t bit_address,
             uint64_t count,
             uint64_t gap_size,
             uint64_t *found_address){

    uint64_t location_in_map = bit_address / 8;
    uint64_t bit_offset, updated_gap_size; 
    uint8_t byte; 
    int rc, bit_count; 
    int aligned_to_byte = bit_address & 0x07;
    int may_have_a_large_gap = 0;


    if( ( bit_address >= total_bits) || 
        ( gap_size == 0)                 || 
        ( count == 0)                    ||
        ( gap_size > count)){ 
        *found_address = 0;
        return( -1);
    }

    if( (bit_address + count) > total_bits){
        count = total_bits - bit_address;
    }

    updated_gap_size = gap_size;
 
    do{
        byte = bm[location_in_map];
        rc = byte_find_gap( aligned_to_byte, 
                            updated_gap_size, 
                            byte, 
                            &bit_count);
                            
        if( ( 8 - aligned_to_byte) > count){
            count = 0;
        }else{
            count -= ( 8 - aligned_to_byte);
        }

        if( rc >= 0){
            if( may_have_a_large_gap == 0){
                bit_offset = rc;
                *found_address = (location_in_map * 8) + bit_offset;
                return(0);
            }else{
                updated_gap_size -= bit_count;
                if( updated_gap_size == 0){
                    return(0);
                } 
            }

        }else if( rc == -1){
            if( (gap_size > count) || ( count == 0)){
                *found_address = 0;
                return( -1);
            }

            updated_gap_size = gap_size; 
            may_have_a_large_gap = 0;

        }else if( rc == -2){
            if( may_have_a_large_gap == 0){
                may_have_a_large_gap = 1;
                *found_address = ((location_in_map + 1) * 8) - bit_count;
            }
            updated_gap_size -= bit_count;

        }
        aligned_to_byte = 0;
        location_in_map++;
    }while( 1);

    /* should not happen */
    return( -2); 
}


/* check if a group of contiguous enabled bits can grow into cleared bits
 * without change location in the bitmap pointed by bm, which has a size
 * of total_bits bits. start is the start address, which already includes
 * some 1s at the beginning. count is the number of bits that we need to
 * resize.
 * Will return:
 * 0: If the extent can grow into trailing zeroes
 * -1 otherwise*/
int bm_extent_can_grow( unsigned char *bm,
                          uint64_t total_bits,
                          uint64_t bit_address,
                          uint64_t count){


    int rc; 
    uint64_t count_result; 
    uint64_t ones, expected_zeroes; 

    if( (  bit_address >= total_bits)         ||
        (( bit_address + count) > total_bits) ||
        (  count == 0) ){

        /* return -1 when the arguments are not valid */
        return( -1);
    }

    /* cover the cases where the map chunk start with 0s. */
    rc = bm_count( bm, total_bits, bit_address, count, 0, &count_result);
    if( rc != 0){

        /* return -100 when something weird happened in bm_count() */
        rc = -100;
        goto bm_extent_exit;

    }else{
        /* we have 0s at beginning.*/ 
        if( count_result > 0){

            /* have just 0s, we can grow, return 0 */
            if( count_result == count){
                /* return 0 if we can grow */
                rc = 0;
            }else{
                /* return -2 if we cant */
                rc = -2;
            }
            goto bm_extent_exit;
        }
    }

    /* count_result == 0 here, and that means we are starting with 1s */
    rc = bm_count( bm, total_bits, bit_address, count, 1, &count_result);

    if( rc != 0){

        /* return -101 when something weird happened in bm_count() */
        rc = -101;
        goto bm_extent_exit; 
    }else{
        if( count_result == count){
            /* return -3 if we got 1s only, not enough room, exit */
            rc = -3;
            goto bm_extent_exit;
        }
    }

    if( count_result == 0){
        /* Should not happen. We should never came here */
        rc = -4;
        goto bm_extent_exit;
    }

    /* at this point we know that we have a map chunk that starts with 1s, 
       and have at least 1 zero after the leading 1s. Let's count how many 
       zeroes do we have */
    ones = count_result; 
    expected_zeroes = count - ones;

    /* do the count */
    rc = bm_count( bm, total_bits, bit_address+ones, expected_zeroes, 0, 
                   &count_result);

    if( rc != 0){
        /* return -101 when something weird happened in bm_count() */
        rc = -102;
        goto bm_extent_exit;
    }

    if( expected_zeroes == count_result){
        /* super good. Have leading 1s and enough room to grow. */
        rc = 0;
    }else{
        /* return -5 if we got 1s first, then 0s and then 1s again. Not
         * enough room, exit */
        rc = -5;
    }
    
bm_extent_exit:
    return( rc);
}



