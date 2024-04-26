#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "krand64.h"

int main(){
    #define DICE_SIDES    100
    #define SAMPLES       100000000

    int i;
    int sample;
    int test_vals[DICE_SIDES];

    memset( &test_vals, 0, sizeof( test_vals));
    set_kseed64(1);
    for( i = 0; i < SAMPLES; i++){ 
        sample = krand64( DICE_SIDES);
        test_vals[sample]++;
    }

    printf("Expected probability: %6.6f\n", 1.0 / (float) DICE_SIDES);
    for( i = 0; i < DICE_SIDES; i++){
        printf("%d : [ %d, %6.6f]\n", i, 
                test_vals[i], 
                ((float) test_vals[i]) / ((float) SAMPLES)); 
    }

    return(0);

}
