#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>



int milisecs_sleep_time_out( uint64_t milisecs, uint64_t timeout_secs){
    struct timespec start_time, sleep_time, current_time, diff_time;
    int rc = 0;
    clock_gettime( CLOCK_MONOTONIC, &start_time);
    current_time = start_time;
    while(1){
        sleep_time = current_time;
        sleep_time.tv_nsec += milisecs * 1000000;
        if( sleep_time.tv_nsec >= 1000000000ul){
            sleep_time.tv_nsec -= 1000000000ul;
            sleep_time.tv_sec++;
        }
        clock_nanosleep( CLOCK_MONOTONIC, TIMER_ABSTIME, &sleep_time, NULL);
        clock_gettime( CLOCK_MONOTONIC, &current_time);

        timespecsub( &current_time, &start_time, &diff_time);
        if( diff_time.tv_sec >= timeout_secs){
            rc = 1;
            break;
        }
        printf("."); fflush(stdout);
    }

    return( rc);
}


int main(int argc, char*argv[])
{
  milisecs_sleep_time_out(500, 10);
}

