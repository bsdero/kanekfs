#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>



struct timespec time_diff( struct timespec const* t1, 
                           struct timespec const* t2){
    struct timespec diff;

    diff.tv_nsec = t1->tv_nsec - t2->tv_nsec;
    diff.tv_sec = t1->tv_sec - t2->tv_sec;
    if( diff.tv_nsec < 0){
        diff.tv_nsec += 1000000000ul;
        diff.tv_sec--;
    }

    return( diff);
}

int milisecs_sleep_time_out( uint64_t milisecs, uint64_t timeout_secs){
    struct timespec start_time, sleep_time, current_time, diff_time1;
    struct timespec start_loop_time, diff_time2;
    int rc = 0;
    clock_gettime( CLOCK_MONOTONIC, &start_time);
    start_loop_time = start_time;
    while(1){
        sleep_time = start_loop_time;
        sleep_time.tv_nsec += 1000000 * milisecs;
        if( sleep_time.tv_nsec >= 1000000000){
            sleep_time.tv_nsec -= 1000000000;
            sleep_time.tv_sec++;
        }
        clock_nanosleep( CLOCK_MONOTONIC, TIMER_ABSTIME, &sleep_time, NULL);
        clock_gettime( CLOCK_MONOTONIC, &current_time);

        timespecsub( &current_time, &start_time, &diff_time2);
        //diff_time1 = time_diff( &current_time, &start_loop_time);
        //diff_time2 = time_diff( &current_time, &start_time);
        printf( "from_loop=%ld.%ld from_beginning=%ld.%ld\n",    
                diff_time1.tv_sec, diff_time1.tv_nsec, 
                diff_time2.tv_sec, diff_time2.tv_nsec );
        fflush( stdout);
        if( diff_time2.tv_sec >= timeout_secs){
            printf("exit\n");
            rc = 1;
            break;
        }
    }

    return( rc);
}


int main(int argc, char*argv[])
{
  milisecs_sleep_time_out(200, 10);
}

