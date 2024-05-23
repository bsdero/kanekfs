#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "map.h"


char *tests[]={
    "SetBits in byte #1",
    "SetBits in byte #2",
    "CountBits in byte #1",
    "CountBits in byte #2",
    "CountBits in byte #3",
    "CountBits in byte #4", 
    "FindGap in byte #1", 
    "FindGap in byte #2", 
    "FindGap in byte #3", 
    "FindGap in byte #4", 
    "SetBits in Bitmap #1", 
    "SetBits in Bitmap #2", 
    "SetBitExtent in Bitmap #1", 
    "SetBitExtent in Bitmap #2", 
    "CountBits in Bitmap #1",
    "CountBits in Bitmap #2",
    "FindGap in Bitmap #1", 
    "FindGap in Bitmap #2", 
    "GrowExtent in Bitmap #1", 
    "GrowExtent in Bitmap #2" 
};


#define TESTS_NUM                                  50
#define FAIL                                       1
#define PASSED                                     0


int global_results[50];
int verbose_mode = 1;


int show_findgap_test_results( int test_id, 
                               int *rcs,
                               uint64_t *addr,
                               int *ercs, 
                               uint64_t *eaddr,
                               int num_of_results){

    int num_of_passed = 0;
    int num_of_fails = 0;
    int test_result = FAIL;
    int i;


    printf("Expected : ");
    for( i = 0; i < num_of_results; i+=2){
        printf( "[%d %lu] ", ercs[i], eaddr[i]);
    }
    printf("\n");

    printf("Got :      ");
    for( i = 0; i < num_of_results; i+=2){
         printf( "[%d %lu] ", rcs[i], addr[i] );
    }
    printf("\n");

    for( i = 0; i < num_of_results; i++){
        if( rcs[i] == ercs[i] && addr[i] == eaddr[i]){
            num_of_passed++;
        } else {
            num_of_fails++;
            printf("Fail in Case #%d : ", i);
            printf("got=[%d, %lu], expected=[%d, %lu]\n", 
                    rcs[i], addr[i], ercs[i], eaddr[i] );
        }
    }

#ifdef DEBUG 
    printf("DEBUG: num_of_fails=%d\n", num_of_fails);
    printf("DEBUG: num_of_passed=%d\n", num_of_passed);
#endif
    if( num_of_fails == 0 && num_of_passed == num_of_results){
        test_result = PASSED;
    }

    global_results[test_id] = test_result;
    printf("%s : %s\n\n", tests[test_id], 
            ( global_results[test_id] ? "FAIL" : "PASSED" ));
}

int show_test_results( int test_id, 
                       int *test_results, 
                       int *expected_results,
                       int num_of_results, 
                       int show_mode){

    int num_of_passed = 0;
    int num_of_fails = 0;
    int test_result = FAIL;
    int i;

    for( i = 0; i < num_of_results; i++){
        if( test_results[i] == expected_results[i]){
            num_of_passed++;
        } else {
            num_of_fails++;
            printf("Fail in Case #%d : ", i);
            printf("got=[%d, 0x%02x], expected=[%d, 0x%02x]\n", 
                    test_results[i], test_results[i], 
                    expected_results[i], expected_results[i]);
        }
    }

#ifdef DEBUG 
    printf("DEBUG: num_of_fails=%d\n", num_of_fails);
    printf("DEBUG: num_of_passed=%d\n", num_of_passed);
#endif
    if( num_of_fails == 0 && num_of_passed == num_of_results){
        test_result = PASSED;
    }

    if( verbose_mode == 1 || global_results[test_id] == FAIL){
        if( show_mode == 0){
            printf("Expected : ");
            for( i = 0; i < num_of_results; i++){
                printf("0x%02x ", expected_results[i]);
            }
            printf("\n");

            printf("Got :      ");
            for( i = 0; i < num_of_results; i++){
                printf("0x%02x ", test_results[i]);
            }
            printf("\n");
        }else if( show_mode == 1){
            printf("Expected : ");
            for( i = 0; i < num_of_results; i+=2){
                printf( "[%d %d] ", 
                        expected_results[i], 
                        expected_results[i+1]);
            }
            printf("\n");

            printf("Got :      ");
            for( i = 0; i < num_of_results; i+=2){
                 printf( "[%d %d] ", 
                        test_results[i], 
                        test_results[i+1]);
            }
            printf("\n");
        }else if( show_mode == 2){
            printf("Expected map: [ ");
            for( i = 0; i < 8; i++){
                printf( "0x%02x ", expected_results[i] );
            }
            printf("\n                ");
            for( i = 8; i < 16; i++){
                printf( "0x%02x ", expected_results[i] );
            }
            printf("]\n");
            printf("Expected return codes: [ ");
            for( ; i < 24; i++){
                 printf( "%02d", expected_results[i]);
                 if( i < 23){
                    printf(", ");    
                 }
            }
            printf(" ]\n");
            printf("Results map:  [ ");
            for( i = 0; i < 8; i++){
                printf( "0x%02x ", test_results[i] );
            }
            printf("\n                ");
            for( i = 8; i < 16; i++){
                printf( "0x%02x ", test_results[i] );
            }
            printf("]\n");
            printf("Results return codes:  [ ");
            for( ; i < 24; i++){
                printf( "%02d", test_results[i]);
                if( i < 23){
                    printf(", ");    
                }
            }
            printf(" ]\n");
        }else if( show_mode == 3){
            printf("Expected map: [ ");
            for( i = 0; i < 8; i++){
                printf( "0x%02x ", expected_results[i] );
            }
            printf("\n                ");
            for( i = 8; i < 16; i++){
                printf( "0x%02x ", expected_results[i] );
            }
            printf("]\n");
            printf("Expected return codes: [ ");
            for( ; i < 28; i++){
                 printf( "%02d", expected_results[i]);
                 if( i < 27){
                    printf(", ");    
                 }
            }
            printf(" ]\n");
            printf("Results map:  [ ");
            for( i = 0; i < 8; i++){
                printf( "0x%02x ", test_results[i] );
            }
            printf("\n                ");
            for( i = 8; i < 16; i++){
                printf( "0x%02x ", test_results[i] );
            }
            printf("]\n");
            printf("Results return codes:  [ ");
            for( ; i < 28; i++){
                printf( "%02d", test_results[i]);
                if( i < 27){
                    printf(", ");    
                }
            }
            printf(" ]\n");
        }else if( show_mode == 4){
            printf("Expected : ");
            for( i = 0; i < num_of_results; i++){
                printf("%02d ", expected_results[i]);
            }
            printf("\n");

            printf("Got :      ");
            for( i = 0; i < num_of_results; i++){
                printf("%02d ", test_results[i]);
            }
            printf("\n");
        }
    }
    global_results[test_id] = test_result;
    printf("%s : %s\n\n", tests[test_id], 
            ( global_results[test_id] ? "FAIL" : "PASSED" ));


    return( test_result);
}


/***********************************************************************
 *
 * MAIN
 ***********************************************************************/
int main(){
    unsigned char m1[16];
    unsigned char m2[16];
    unsigned char b0 = 0x00;
    unsigned char bresult[16];
    int iresult[16], icount[16], ioffset[16], igap[16], erc[16];
    int i, rc;
    uint64_t count_result[16], addr[16], eaddr[16]; 
    int test_id = 0;
    
    memset( &global_results, 0, sizeof( int) * TESTS_NUM);

    /********************************************************************
     * SET BIT IN BYTE TESTS
     * set or clear numbits bits in the byte passed as argument, starting in 
     * the start bit. Return the updated byte.
     *
     * unsigned char byte_set_bits( int start,
     *                              int numbits,
     *                              char byte,
     *                              int clear_or_set){
     *
     ********************************************************************/
 
    /*********************************************************************
     * TEST #1
     * SetBits in byte #1 
     */
    int e_results_1[8] = { 0x01, 0x03, 0x0f, 0x7f, 0xff, 0xff, 0x04, 0x0c };
    int a_results[32];
    if( verbose_mode == 1){
        printf("Test #%d: %s\n", test_id, tests[test_id]);
    }


    bresult[0] = byte_set_bits( 0, 1, 0x00, 1);
    bresult[1] = byte_set_bits( 0, 2, 0x00, 1);
    bresult[2] = byte_set_bits( 0, 4, 0x00, 1);
    bresult[3] = byte_set_bits( 0, 7, 0x00, 1);
    bresult[4] = byte_set_bits( 0, 8, 0x00, 1);
    bresult[5] = byte_set_bits( 0, 22, 0x00, 1);
    bresult[6] = byte_set_bits( 2, 1, 0x00, 1);
    bresult[7] = byte_set_bits( 2, 2, 0x00, 1);

    for( i = 0; i < 8; i++){
        a_results[i] = (int) bresult[i];
    }

    show_test_results( test_id, a_results, e_results_1, 8, 0);
    test_id++;
    /*********************************************************************
     * TEST #2
     * SetBits in byte #2
     */
    int e_results_2[8] = { 0xfe, 0xfc, 0xf0, 0x80, 0x00, 0x00, 0xfb, 0xf3};
    if( verbose_mode == 1){
        printf("%d: %s\n", test_id, tests[test_id]);
    }

    bresult[0] = byte_set_bits( 0, 1, 0xff, 0);
    bresult[1] = byte_set_bits( 0, 2, 0xff, 0);
    bresult[2] = byte_set_bits( 0, 4, 0xff, 0);
    bresult[3] = byte_set_bits( 0, 7, 0xff, 0);
    bresult[4] = byte_set_bits( 0, 8, 0xff, 0);
    bresult[5] = byte_set_bits( 0, 22, 0xff, 0);
    bresult[6] = byte_set_bits( 2, 1, 0xff, 0);
    bresult[7] = byte_set_bits( 2, 2, 0xff, 0);
 
    for( i = 0; i < 8; i++){
        a_results[i] = (int) bresult[i];
    }

    show_test_results( test_id, a_results, e_results_2, 8, 0);
    test_id++;
    /********************************************************************
     * COUNT BITS TESTS
     *
     * count how many contiguous bits has the byte passed as argument,
     * starting in the startbit, counting max of numbits. clear_or_set
     * specifies if we are looking for contiguous 0s or 1s. Will return
     * the number of contiguous bits 
     *
     * int byte_count_bits( int start, int numbits, 
     *                      char byte, int clear_or_set);
     ********************************************************************/
 
    /*********************************************************************
     * TEST #3
     * CountBits in byte #1
     */
    int e_results_3[8] = { 8, 0, 1, 0, 6, 0, 1, 0 };
    if( verbose_mode == 1){
        printf("Test #%d: %s\n", test_id, tests[test_id]);
    }

    iresult[0] = byte_count_bits( 0, 8, 0x00, 0);
    iresult[1] = byte_count_bits( 0, 8, 0x00, 1);
    iresult[2] = byte_count_bits( 0, 1, 0x00, 0);
    iresult[3] = byte_count_bits( 0, 1, 0x00, 1); 
    iresult[4] = byte_count_bits( 2, 8, 0x00, 0);
    iresult[5] = byte_count_bits( 2, 8, 0x00, 1);
    iresult[6] = byte_count_bits( 2, 1, 0x00, 0);
    iresult[7] = byte_count_bits( 2, 1, 0x00, 1);

    show_test_results( test_id, iresult, e_results_3, 8, 0);
    test_id++;

    /*********************************************************************
     * TEST #4
     * CountBits in byte #2 
     */
    int e_results_4[8] = { 0, 8, 0, 1, 0, 6, 0, 1 };
    if( verbose_mode == 1){
        printf("Test #%d: %s\n", test_id, tests[test_id]);
    }

    iresult[0] = byte_count_bits( 0, 8, 0xff, 0);
    iresult[1] = byte_count_bits( 0, 8, 0xff, 1);
    iresult[2] = byte_count_bits( 0, 1, 0xff, 0);
    iresult[3] = byte_count_bits( 0, 1, 0xff, 1);
    iresult[4] = byte_count_bits( 2, 8, 0xff, 0);
    iresult[5] = byte_count_bits( 2, 8, 0xff, 1);
    iresult[6] = byte_count_bits( 2, 1, 0xff, 0);
    iresult[7] = byte_count_bits( 2, 1, 0xff, 1);

    show_test_results( test_id, iresult, e_results_4, 8, 0);
    test_id++;

    /*********************************************************************
     * TEST #5
     * CountBits in byte #3 
     */
    int e_results_5[8] = { 0, 4, 0, 1, 0, 2, 0, 1 };
    if( verbose_mode == 1){
        printf("Test #%d: %s\n", test_id, tests[test_id]);
    }

    iresult[0] = byte_count_bits( 0, 8, 0x0f, 0);
    iresult[1] = byte_count_bits( 0, 8, 0x0f, 1);
    iresult[2] = byte_count_bits( 0, 1, 0x0f, 0);
    iresult[3] = byte_count_bits( 0, 1, 0x0f, 1);
    iresult[4] = byte_count_bits( 2, 8, 0x0f, 0);
    iresult[5] = byte_count_bits( 2, 8, 0x0f, 1);
    iresult[6] = byte_count_bits( 2, 1, 0x0f, 0);
    iresult[7] = byte_count_bits( 2, 1, 0x0f, 1);

    show_test_results( test_id, iresult, e_results_5, 8, 0);
    test_id++;

    /*********************************************************************
     * TEST #6
     * CountBits in byte #4 
     */
    int e_results_6[8] = { 4, 0, 1, 0, 2, 0, 1, 0 };
    if( verbose_mode == 1){
        printf("Test #%d: %s\n", test_id, tests[test_id]);
    }


    iresult[0] = byte_count_bits( 0, 8, 0xf0, 0);
    iresult[1] = byte_count_bits( 0, 8, 0xf0, 1);
    iresult[2] = byte_count_bits( 0, 1, 0xf0, 0);
    iresult[3] = byte_count_bits( 0, 1, 0xf0, 1);
    iresult[4] = byte_count_bits( 2, 8, 0xf0, 0);
    iresult[5] = byte_count_bits( 2, 8, 0xf0, 1);
    iresult[6] = byte_count_bits( 2, 1, 0xf0, 0);
    iresult[7] = byte_count_bits( 2, 1, 0xf0, 1);

    show_test_results( test_id, iresult, e_results_6, 8, 0);
    test_id++;

    /********************************************************************
     *  FIND GAP IN BYTE TESTS
     * 
     * find a gap of numbits, starting in start, in the byte passed as
     *  argument. Will return the offset of a found gap, *count will be
     *  updated with the number of contiguous bits found.
     *  Will return: 
     *   0 or greater: offset of a found gap
     *  -1 if no gap was found, *count will be  = 0
     *  -2 if the end of byte was reached and we did not found enough
     *      numbits bits. *count will have the number of bits found 
     *  int byte_find_gap( int start, int numbits, 
     *                     char byte, int *count);
     */

    /*********************************************************************
     * TEST #7
     * FindGap in Byte #1 
     */
    int gap_results[16];

    int e_results_7[16] = { 
         0,  1,  0,  4,  0,  8, -2,  8, 
        -1,  0, -1,  0, -1,  0, -1,  0};

    if( verbose_mode == 1){
        printf("Test #%d: %s\n", test_id, tests[test_id]);
    }

    iresult[0] = byte_find_gap( 0, 1, 0x00, &icount[0]);
    iresult[1] = byte_find_gap( 0, 4, 0x00, &icount[1]);
    iresult[2] = byte_find_gap( 0, 8, 0x00, &icount[2]);
    iresult[3] = byte_find_gap( 0, 12, 0x00, &icount[3]);
    iresult[4] = byte_find_gap( 0, 1, 0xff, &icount[4]);
    iresult[5] = byte_find_gap( 0, 4, 0xff, &icount[5]);
    iresult[6] = byte_find_gap( 0, 8, 0xff, &icount[6]);
    iresult[7] = byte_find_gap( 0, 12, 0xff, &icount[7]);
    for( i = 0; i < 8; i++){
        gap_results[i*2] = iresult[i];
        gap_results[i*2+1] = icount[i];
    }

    show_test_results( test_id, gap_results, e_results_7, 16, 1);
    test_id++;

    /*********************************************************************
     * TEST #8
     * FindGap in Byte #2 
     */
    int e_results_8[16] = { 
         4,  1,  4,  4, -2,  4, -2,  4, 
         0,  1,  0,  4, -1,  0, -1,  0};

    if( verbose_mode == 1){
        printf("Test #%d: %s\n", test_id, tests[test_id]);
    }

    iresult[0] = byte_find_gap( 0, 1, 0x0f, &icount[0]);
    iresult[1] = byte_find_gap( 0, 4, 0x0f, &icount[1]);
    iresult[2] = byte_find_gap( 0, 8, 0x0f, &icount[2]);
    iresult[3] = byte_find_gap( 0, 12, 0x0f, &icount[3]);
    iresult[4] = byte_find_gap( 0, 1, 0xf0, &icount[4]);
    iresult[5] = byte_find_gap( 0, 4, 0xf0, &icount[5]);
    iresult[6] = byte_find_gap( 0, 8, 0xf0, &icount[6]);
    iresult[7] = byte_find_gap( 0, 12, 0xf0, &icount[7]);
    for( i = 0; i < 8; i++){
        gap_results[i*2] = iresult[i];
        gap_results[i*2+1] = icount[i];
    }

    show_test_results( test_id, gap_results, e_results_8, 16, 1);
    test_id++;

    /*********************************************************************
     * TEST #9
     * FindGap in Byte #3
     */
    int e_results_9[16] = { 
         1,  1, -2,  1,  1,  1, -2,  1, 
         1,  1,  5,  2,  3,  2,  3,  3};

    if( verbose_mode == 1){
        printf("Test #%d: %s\n", test_id, tests[test_id]);
    }

    iresult[0] = byte_find_gap( 0, 1, 0x55, &icount[0]);
    iresult[1] = byte_find_gap( 0, 2, 0x55, &icount[1]);
    iresult[2] = byte_find_gap( 1, 1, 0x55, &icount[2]);
    iresult[3] = byte_find_gap( 1, 2, 0x55, &icount[3]);
    iresult[4] = byte_find_gap( 0, 1, 0x95, &icount[4]);
    iresult[5] = byte_find_gap( 0, 2, 0x95, &icount[5]);
    iresult[6] = byte_find_gap( 0, 2, 0x45, &icount[6]);
    iresult[7] = byte_find_gap( 0, 3, 0x45, &icount[7]);
    for( i = 0; i < 8; i++){
        gap_results[i*2] = iresult[i];
        gap_results[i*2+1] = icount[i];
    }

    show_test_results( test_id, gap_results, e_results_9, 16, 1);
    test_id++;

    /*********************************************************************
     * TEST #10
     * FindGap in Byte #4
     */
    int e_results_10[16] = { 
         5,  1, -2,  1,  4,  1,  4,  4, 
        -2,  4, -1,  0, -1,  0, -2,  1};

    if( verbose_mode == 1){
        printf("Test #%d: %s\n", test_id, tests[test_id]);
    }

    iresult[0] = byte_find_gap( 4, 1, 0x55, &icount[0]);
    iresult[1] = byte_find_gap( 4, 2, 0x55, &icount[1]);
    iresult[2] = byte_find_gap( 4, 1, 0x00, &icount[2]);
    iresult[3] = byte_find_gap( 4, 4, 0x00, &icount[3]);
    iresult[4] = byte_find_gap( 4, 6, 0x00, &icount[4]);
    iresult[5] = byte_find_gap( 4, 1, 0xff, &icount[5]);
    iresult[6] = byte_find_gap( 4, 4, 0xff, &icount[6]);
    iresult[7] = byte_find_gap( 7, 2, 0x00, &icount[7]);
    for( i = 0; i < 8; i++){
        gap_results[i*2] = iresult[i];
        gap_results[i*2+1] = icount[i];
    }

    show_test_results( test_id, gap_results, e_results_10, 16, 1);
    test_id++;

    /********************************************************************
     * SET BIT IN BITMAP
     *
     * set one bit in the blockmap pointed by bm, which has total_blocks bits.
     * block_address is the bit to change and clear_or_set is a flag to set
     * or clear the bit. 
     * int bm_set_bit( unsigned char *bm,
     *                 uint64_t total_blocks,
     *                 uint64_t block_address,
     *                 int clear_or_set);
     */

    /*********************************************************************
     * TEST #11
     * Set bit in bitmap #1
     */
    /* expected bitmap */
    unsigned char e_rs_11[16] = { 0x8a, 0x81, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x82,
                                  0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00};
                                  
    /* expected return codes */                              
    int e_rc_11[8]=             {    0,    0,    0,    0,  
                                     0,    0,    0,   -1};

    /* store results here ( bitmap byte values and return codes) */
    int ie_rs_11[24];

    /* store expected results here */
    int i_rs_11[24];

    /* fill in expected results with expected map and expected return codes */
    for( i = 0; i < 16; i++){
        ie_rs_11[i] = e_rs_11[i];
    }
    for( i = 0; i < 8; i++){
        ie_rs_11[i+16] = e_rc_11[i];
    }

    if( verbose_mode == 1){
        printf("Test #%d: %s\n", test_id, tests[test_id]);
    }

    memset( (void *) &m1, 0, sizeof( m1));
 
    icount[0] = 1;
    icount[1] = 3;
    icount[2] = 7;
    icount[3] = 8;
    icount[4] = 15;
    icount[5] = 57;
    icount[6] = 63;
    icount[7] = 64;

    for( i = 0; i < 8; i++){
        i_rs_11[i+16] = bm_set_bit( m1, 64, icount[i], 1);
    }
    for( i = 0; i < 16; i++){
        i_rs_11[i] = (int) m1[i];
    }
    show_test_results( test_id, i_rs_11, ie_rs_11, 24, 2);
    test_id++;

    /*********************************************************************
     * TEST #12
     * Set bit in bitmap #2
     */
    unsigned char e_rs_12[16] = { 0x75, 0x7e, 0xff, 0xff,
                                  0xff, 0xff, 0xff, 0x7d,
                                  0xff, 0xff, 0xff, 0xff,
                                  0xff, 0xff, 0xff, 0xff };

    int e_rc_12[8]=             {    0,    0,    0,    0,  
                                     0,    0,    0,   -1};

    /* store results here ( bitmap byte values and return codes) */
    int ie_rs_12[24];

    /* store expected results here */
    int i_rs_12[24];

    /* fill in expected results with expected map and expected return codes */
    for( i = 0; i < 16; i++){
        ie_rs_12[i] = e_rs_12[i];
    }
    for( i = 0; i < 8; i++){
        ie_rs_12[i+16] = e_rc_12[i];
    }

    if( verbose_mode == 1){
        printf("Test #%d: %s\n", test_id, tests[test_id]);
    }

    memset( (void *) &m1, 0xff, sizeof( m1));
 
    icount[0] = 1;
    icount[1] = 3;
    icount[2] = 7;
    icount[3] = 8;
    icount[4] = 15;
    icount[5] = 57;
    icount[6] = 63;
    icount[7] = 64;

    for( i = 0; i < 8; i++){
        i_rs_12[i+16] = bm_set_bit( m1, 64, icount[i], 0);
    }
    for( i = 0; i < 16; i++){
        i_rs_12[i] = (int) m1[i];
    }
    show_test_results( test_id, i_rs_12, ie_rs_12, 24, 2);
    test_id++;

    /********************************************************************
     * SET BIT EXTENT IN BITMAP
     * A bit extent is a group of consecutive bits with the same value. 
     * This tests check the functionality of the bm_set_extent() function.
     *
     * This function sets a group of continuous bits in the blockmap 
     * pointed by bm, which has bits_len length bits. Offset is the 
     * start of the contiguous bits extent to update and clear_or_set 
     * is a flag to set or clear the bits. The number of contiguous bits
     * to set is num_bits_to_set. 
     *  int bm_set_extent( unsigned char *bm,
     *              uint64_t bits_len,
     *              uint64_t offset,
     *              uint64_t num_bits_to_set,
     *              int clear_or_set);
     */


    /*********************************************************************
     * TEST #13
     * Set bit extent in bitmap #1
     */
     /* expected bitmap */
    unsigned char e_rs_13[16] = { 0xb9, 0x00, 0xff, 0x00,
                                  0xff, 0x0f, 0xff, 0xff,
                                  0x0f, 0xfc, 0xf0, 0x0f,
                                  0xf0, 0xff, 0x0f, 0x80 };
    /* expected return codes */                              
    int e_rc_13[12] =           {    0,    0,    0,    0,
                                     0,    0,    0,    0,
                                     0,   -1,    0,   -1 };

    /* store results here ( bitmap byte values and return codes) */
    int ie_rs_13[28];

    /* store expected results here */
    int i_rs_13[28];

    /* fill in expected results with expected map and expected return codes */
    for( i = 0; i < 16; i++){
        ie_rs_13[i] = e_rs_13[i];
    }
    for( i = 0; i < 12; i++){
        ie_rs_13[i+16] = e_rc_13[i];
    }
    memset( (void *) &m1, 0, sizeof( m1));

    if( verbose_mode == 1){
        printf("Test #%d: %s\n", test_id, tests[test_id]);
    }

    ioffset[0] = 0; icount[0] = 1;
    ioffset[1] = 3; icount[1] = 3;
    ioffset[2] = 7; icount[2] = 1;
    ioffset[3] = 16; icount[3] = 8;
    ioffset[4] = 32; icount[4] = 12;
    ioffset[5] = 48; icount[5] = 20;
    ioffset[6] = 74; icount[6] = 6; 
    ioffset[7] = 84; icount[7] = 8; 
    ioffset[8] = 100; icount[8] = 16;
    ioffset[9] = 124; icount[9] = 8;
    ioffset[10] = 127; icount[10] = 1; 
    ioffset[11] = 128; icount[11] = 1;

    for( i = 0; i < 12; i++){
        rc = bm_set_extent( m1, 128, ioffset[i], icount[i], 1);
        i_rs_13[i+16] = rc;
/*        if( rc < 0){
            printf("Error: case=%d, locat=%d + ext =%d outside map size=%d\n",
            i, 
            ioffset[i], 
            icount[i], 
            128);
        }
*/
    }

    for( i = 0; i < 16; i++){
        i_rs_13[i] = (int) m1[i];
    }
    show_test_results( test_id, i_rs_13, ie_rs_13, 28, 3);
    test_id++;

     /*********************************************************************
     * TEST #14
     * Set bit extent in bitmap #2
     */
     /* expected bitmap */
    unsigned char e_rs_14[16] = { 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00 };
    /* expected return codes */                              
    int e_rc_14[12] =           {    0,    0,    0,    0,
                                     0,    0,    0,    0,
                                     0,   -1,    0,   -1 };

    /* store results here ( bitmap byte values and return codes) */
    int ie_rs_14[28];

    /* store expected results here */
    int i_rs_14[28];

    /* fill in expected results with expected map and expected return codes */
    for( i = 0; i < 16; i++){
        ie_rs_14[i] = e_rs_14[i];
    }
    for( i = 0; i < 12; i++){
        ie_rs_14[i+16] = e_rc_14[i];
    }

    if( verbose_mode == 1){
        printf("Test #%d: %s\n", test_id, tests[test_id]);
    }

    for( i = 0; i < 12; i++){
        rc = bm_set_extent( m1, 128, ioffset[i], icount[i], 0);
        i_rs_14[i+16] = rc;
/*        if( rc < 0){
            printf("Error: case=%d, locat=%d + ext =%d outside map size=%d\n",
            i, 
            ioffset[i], 
            icount[i], 
            128);
        }
*/
    }

    for( i = 0; i < 16; i++){
        i_rs_14[i] = (int) m1[i];
    }
    show_test_results( test_id, i_rs_14, ie_rs_14, 28, 3);
    test_id++;



     /********************************************************************
     * BIT COUNT IN BITMAP
     * This tests check the functionality of the bm_count() function.
     *
     * This function count how many contiguous 0 or 1 do we have in 
     * the bitmap pointed by bm, which has a size of bitmap_len bits. 
     * The count starts in offset, until num_bits_to_count bits is reached.
     * clear_or_set is a flag to count contiguous 1s or 0s.
     *
     * int bm_count( unsigned char *bm,
     *               uint64_t bitmap_len,
     *               uint64_t offset,
     *               uint64_t num_bits_to_count,
     *               int clear_or_set,
     *               uint64_t *count_result)
     */


    /*********************************************************************
     * TEST #15
     * BitCount in bitmap #1
     */
    unsigned char m15[] = { 0xff, 0xfe, 0xfc, 0xf0, 0x00, 0x00, 0xf0, 0x00,
                            0x00, 0xf0, 0x00, 0x00, 0x00, 0xf0, 0xff, 0x00,
                            0x00, 0x00, 0x00, 0x00};

    int i_e_15[18] = {  0,  0,  0,  1,  0,  2,  
                        0,  4,  0,  8,  0, 12,  
                        0, 20,  0, 28, -1,  0 };

    int i_r_15[18];

    if( verbose_mode == 1){
        printf("Test #%d: %s\n", test_id, tests[test_id]);
    }

    ioffset[0] = 0;  icount[0] = 8;
    ioffset[1] = 8;  icount[1] = 8;
    ioffset[2] = 16; icount[2] = 8;
    ioffset[3] = 24; icount[3] = 8;
    ioffset[4] = 32; icount[4] = 8;
    ioffset[5] = 40; icount[5] = 16;
    ioffset[6] = 56; icount[6] = 24;
    ioffset[7] = 80; icount[7] = 40;
    ioffset[8] = 120; icount[8] = 100;

    for( i = 0; i < 9; i++){
        rc = bm_count( m15, 160, ioffset[i], icount[i], 0, &count_result[i]);
/*        if( rc < 0){
            printf("Error, location=%d + extent=%d outside map size=%d\n",
            ioffset[i],
            icount[i],
            128);
        }*/
        iresult[i] = rc;
        i_r_15[2*i] = iresult[i];
        i_r_15[2*i+1] = count_result[i];
    }

    show_test_results( test_id, i_r_15, i_e_15, 18, 4);
    test_id++;


    /*********************************************************************
     * TEST #16
     * BitCount in bitmap #2
     */
    unsigned char m16[] = { 0xff, 0xfc, 0xf0, 0x00, 
                            0x00, 0x00, 0xf0, 0x0f,
                            0x00, 0xf0, 0x00, 0x00, 
                            0x00, 0xf0, 0xff, 0x00,
                            0x00, 0x00, 0xff, 0x00,
                          };

    int i_e_16[20] = {  0,  0,  0,  1,  0,  3,  
                        0,  8,  0, 19,  0, 11,  
                        0, 16,  0, 27, -1,  0,
                        0,  5};

    int i_r_16[20];

    if( verbose_mode == 1){
        printf("Test #%d: %s\n", test_id, tests[test_id]);
    }

                                       /* expected : */
    ioffset[0] = 1;   icount[0] = 8;     /* 0 0 */
    ioffset[1] = 9;   icount[1] = 8;     /* 0 1 */ 
    ioffset[2] = 17;  icount[2] = 8;     /* 0 3 */
    ioffset[3] = 25;  icount[3] = 8;     /* 0 8 */
    ioffset[4] = 33;  icount[4] = 20;    /* 0 19 */
    ioffset[5] = 41;  icount[5] = 16;    /* 0 11 */
    ioffset[6] = 60;  icount[6] = 24;    /* 0 16 */
    ioffset[7] = 81;  icount[7] = 40;    /* 0 27 */
    ioffset[8] = 121; icount[8] = 100;  /* -1 0 */
    ioffset[9] = 27;  icount[9] = 5;     /* 0 5 */

    for( i = 0; i < 10; i++){
        rc = bm_count( m16, 160, 
                       ioffset[i], icount[i], 
                       0, &count_result[i]);
/*
        if( rc < 0){
            printf("Error, location=%d + extent=%d outside map size=%d\n",
            ioffset[i],
            icount[i],
            128);
        }
*/
        iresult[i] = rc;
        i_r_16[2*i] = iresult[i];
        i_r_16[2*i+1] = count_result[i];
    }

    show_test_results( test_id, i_r_16, i_e_16, 20, 4);
    test_id++;



     /********************************************************************
     * FIND GAP IN BITMAP
     * This tests check the functionality of the bm_find() function.
     *
     * This function find a gap of contiguous zeroed bits in the bitmap 
     * pointed by bm, which has a size of bitmap_len bits. We start to look 
     * for a gap in the address pointed by start, and will stop after count 
     * bits were checked or until the total_blocks is reached. The size of 
     * the gap in bits is specified in gap_size.
     *
     * If the gap was found, return 0 and set found_adress to the bit offset
     * in the map where the gap is.
     * If no gap was found, return -1.  
     * 
     * int bm_find( unsigned char *bm,
     *              uint64_t total_bits,
     *              uint64_t start,
     *              uint64_t count,
     *              uint64_t gap_size,
     *              uint64_t *found_address);
     */              

    /*********************************************************************
     * TEST #17
     * Find Bit Gap in bitmap #1
     */

    unsigned char m17[] = { 0xff, 0xff, 0x0f, 0x00, 
                            0x00, 0xf0, 0xff, 0xff,
                            0x00, 0xf0, 0x00, 0x00, 
                            0x9d, 0xff, 0xfd, 0xf9,
                            0xf0, 0xff, 0xff, 0xff,
                          };

    int i_erc_17[] = { -1, -1,  0,  0,  0,  0,  0,  0,
                        0,  0,  0,  0,  0,  0,  0,  0 };

    uint64_t i_eaddr_17[] =  {  0,  0, 20, 20, 20, 20, 20, 20,
                               20, 20, 20, 20, 20, 20, 20, 20}; 

    int i_rc_17[16];

    if( verbose_mode == 1){
        printf("Test #%d: %s\n", test_id, tests[test_id]);
    }

    ioffset[0] = 0;  icount[0] = 8;   igap[0] = 1; 
    ioffset[1] = 2;  icount[1] = 8;   igap[1] = 1; 
    ioffset[2] = 0;  icount[2] = 22;  igap[2] = 1; 
    ioffset[3] = 2;  icount[3] = 22;  igap[3] = 2;
    ioffset[4] = 0;  icount[4] = 64;  igap[4] = 4; 
    ioffset[5] = 0;  icount[5] = 64;  igap[5] = 6; 
    ioffset[6] = 0;  icount[6] = 64;  igap[6] = 12; 
    ioffset[7] = 0;  icount[7] = 64;  igap[7] = 16; 
    ioffset[8] = 0;  icount[8] = 64;  igap[8] = 20; 
    ioffset[9] = 0;  icount[9] = 64;  igap[9] = 22; 
    ioffset[10] = 2; icount[10] = 64; igap[10] = 4; 
    ioffset[11] = 2; icount[11] = 64; igap[11] = 6; 
    ioffset[12] = 2; icount[12] = 64; igap[12] = 12;
    ioffset[13] = 2; icount[13] = 64; igap[13] = 16;
    ioffset[14] = 2; icount[14] = 64; igap[14] = 20;
    ioffset[15] = 2; icount[15] = 64; igap[15] = 22;


    for( i = 0; i < 16; i++){
        rc = bm_find( m17, 128, ioffset[i], icount[i], igap[i], &addr[i]);
/*        printf( "Case #%d, expected values = [%d %lu], got: [%d %lu] ", 
                i ,erc[i], eaddr[i], rc, addr[i]);

        if( (rc == erc[i]) && ( eaddr[i] == addr[i] )){
            printf("OK");
        }else{
            printf("ERROR");
        }
        printf("\n");
*/
        i_rc_17[i] = rc;
    }

    show_findgap_test_results( test_id, i_rc_17, addr, 
                               i_erc_17, i_eaddr_17, 16);

    test_id++;

    /*********************************************************************
     * TEST #18
     * Find Bit Gap in bitmap #2
     */

    unsigned char m18[] = { 0xff, 0x7f, 0xff, 0x9d, 
                            0x88, 0xff, 0x0f, 0xff,
                            0xe0, 0x00, 0x00, 0x00, 
                            0x00, 0x00, 0x00, 0xf0,
                          };

    int i_erc_18[] = {  0,  0,  0,  0,  0,  0,  0,  0, 
                       -1,  0, -1, -1 };

    uint64_t i_eaddr_18[] =  { 15, 15, 25, 29, 32, 52, 64, 72,
                                0,120,  0,  0}; 

    int i_rc_18[12];

    if( verbose_mode == 1){
        printf("Test #%d: %s\n", test_id, tests[test_id]);
    }


    ioffset[0] = 0;    icount[0] = 24;  igap[0] = 1; 
    ioffset[1] = 4;    icount[1] = 24;  igap[1] = 1;
    ioffset[2] = 16;   icount[2] = 64;  igap[2] = 1;
    ioffset[3] = 16;   icount[3] = 64;  igap[3] = 2;
    ioffset[4] = 0;    icount[4] = 64;  igap[4] = 3;
    ioffset[5] = 0;    icount[5] = 64;  igap[5] = 4;
    ioffset[6] = 0;    icount[6] = 128; igap[6] = 5;
    ioffset[7] = 0;    icount[7] = 128; igap[7] = 50;
    ioffset[8] = 0;    icount[8] = 128; igap[8] = 64;
    ioffset[9] = 120;  icount[9] = 64;  igap[9] = 4;
    ioffset[10] = 120; icount[10] = 64; igap[10] = 8;
    ioffset[11] = 120; icount[11] = 64; igap[11] = 12;

    for( i = 0; i < 12; i++){
        rc = bm_find( m18, 128, ioffset[i], icount[i], igap[i], &addr[i]);
/*        printf( "Case #%d, expected values = [%d %lu], got: [%d %lu] ",
                i ,i_erc_18[i], i_eaddr_18[i], rc, addr[i]);

        if( (rc == i_erc_18[i]) && ( i_eaddr_18[i] == addr[i] )){
            printf("OK");
        }else{
            printf("ERROR");
        }
        printf("\n");*/
        i_rc_18[i] = rc;
    }
    show_findgap_test_results( test_id, i_rc_18, addr, 
                               i_erc_18, i_eaddr_18, 12);

    test_id++;


     /********************************************************************
     * GROW EXTENT IN BITMAP
     * This tests check the functionality of the bm_extent_can_grow() 
     * function. 
     *
     * The function bm_extent_can_grow() checks if a group of contiguous 
     * enabled bits can grow into cleared bits, without change location 
     * in the blockmap pointed by bm, which has a size of total_blocks bits.
     * start is the start address, which already includes some 1s at the 
     * beginning. count is the size of the bits chunk we need to resize.
     * Will return:
     * 0: If the extent can grow into trailing zeroes in the bitmap
     * -1 otherwise
     *
     * int bm_extent_can_grow( unsigned char *bm,
     *                         uint64_t total_blocks,
     *                         uint64_t block_address,
     *                         uint64_t count){
     */

    /*********************************************************************
     * TEST #19
     * Grow extent in bitmap #1
     */


    unsigned char m19[] = { 0x07, 0xff, 0xff, 0xff, 
                            0x3f, 0x00, 0x00, 0x00,
                            0x0f, 0xff, 0x3f, 0xff, 
                            0x3f, 0x00, 0x00, 0xf0,
                          };

    int i_erc_19[] = { -1, -3,  0,  0, -5, -3, -3,  0, 
                        0,  0, -2,  0,  0,  0,  0,  0 };

    int i_rc_19[15];
    if( verbose_mode == 1){
        printf("Test #%d: %s\n", test_id, tests[test_id]);
    }
   
    ioffset[0] = 0;    icount[0] = 0; 
    ioffset[1] = 0;    icount[1] = 2;
    ioffset[2] = 0;    icount[2] = 5;
    ioffset[3] = 0;    icount[3] = 8;
    ioffset[4] = 0;    icount[4] = 12;
    ioffset[5] = 10;   icount[5] = 5;
    ioffset[6] = 10;   icount[6] = 20;
    ioffset[7] = 10;   icount[7] = 38;
    ioffset[8] = 39;   icount[8] = 1;
    ioffset[9] = 39;   icount[9] = 20;
    ioffset[10] = 39;  icount[10] = 32;
    ioffset[11] = 40;  icount[11] = 5;
    ioffset[12] = 40;  icount[12] = 8;
    ioffset[13] = 40;  icount[13] = 10;
    ioffset[14] = 40;  icount[14] = 15;
    ioffset[15] = 40;  icount[15] = 16;
    
    for( i = 0; i < 16; i++){
        rc = bm_extent_can_grow( m19, 128, ioffset[i], icount[i]);
        i_rc_19[i] = rc;
        /*
        printf( "Case #%d, expected rc=%d, got: %d ",
                i, erc[i], rc);

        if( rc == erc[i]){
            printf("OK");
        }else{
            printf("ERROR");
        }
        printf("\n");
        */
    }

    show_test_results( test_id, i_rc_19, i_erc_19, 16, 4);
    test_id++;

    /*********************************************************************
     * TEST #19
     * Grow extent in bitmap #1
     */


    unsigned char m20[] = { 0x07, 0xff, 0xff, 0xff, 
                            0x3f, 0x00, 0x00, 0x00,
                            0x0f, 0xff, 0x3f, 0xff, 
                            0x3f, 0x00, 0x00, 0xf0,
                          };

    int i_erc_20[] = { -5, -5, -5,  0,  
                        0,  0, -2, -2, 
                       -2, -3, -3, -3 };

    int i_rc_20[15];
    if( verbose_mode == 1){
        printf("Test #%d: %s\n", test_id, tests[test_id]);
    }
   
    ioffset[0] = 24;   icount[0] = 48;
    ioffset[1] = 25;   icount[1] = 48;
    ioffset[2] = 23;   icount[2] = 48;
    ioffset[3] = 24;   icount[3] = 24;
    ioffset[4] = 25;   icount[4] = 25;
    ioffset[5] = 23;   icount[5] = 24;
    ioffset[6] = 47;   icount[6] = 24;
    ioffset[7] = 48;   icount[7] = 24;
    ioffset[8] = 49;   icount[8] = 24;
    ioffset[9] = 15;   icount[9] = 16;
    ioffset[10] = 16;  icount[10] = 16;
    ioffset[11] = 17;  icount[11] = 16;

    for( i = 0; i < 16; i++){
        rc = bm_extent_can_grow( m20, 128, ioffset[i], icount[i]);
        i_rc_20[i] = rc;
        /*
        printf( "Case #%d, expected rc=%d, got: %d ",
                i, erc[i], rc);

        if( rc == erc[i]){
            printf("OK");
        }else{
            printf("ERROR");
        }
        printf("\n");
        */
    }

    show_test_results( test_id, i_rc_20, i_erc_20, 12, 4);
    test_id++;

    printf("Resume: \n");
    for( i = 0; i < test_id; i++){
        printf("Test #%d, %s : %s\n", i, tests[i], 
            ( global_results[i] ? "FAIL" : "PASSED" ));
    }
    printf("\n");
    return(0);
}


