#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

#define CLOCK CLOCK_MONOTONIC
//#define CLOCK CLOCK_REALTIME
//#define CLOCK CLOCK_TAI
//#define CLOCK CLOCK_BOOTTIME


static long calcTimeDiff(struct timespec const* t1, struct timespec const* t2)
{
  long diff = t1->tv_nsec - t2->tv_nsec;
  diff += 1000000000 * (t1->tv_sec - t2->tv_sec);
  return diff;
}

static void* tickThread()
{
  struct timespec sleepStart;
  struct timespec currentTime;
  struct timespec wakeTime;
  long sleepTime;
  long wakeDelay;


  while(1)
  {
      clock_gettime(CLOCK, &wakeTime);
      wakeTime.tv_sec += 1;
      wakeTime.tv_nsec -= 0; //  Value to play with for delay "correction"
    
    clock_gettime(CLOCK, &sleepStart);
    clock_nanosleep(CLOCK, TIMER_ABSTIME, &wakeTime, NULL);
    clock_gettime(CLOCK, &currentTime);
    sleepTime = calcTimeDiff(&currentTime, &sleepStart);
    wakeDelay = calcTimeDiff(&currentTime, &wakeTime);
    {
      /*printf("sleep req=%-ld.%-ld start=%-ld.%-ld curr=%-ld.%-ld sleep=%-ld delay=%-ld\n",
          (long) wakeTime.tv_sec, (long) wakeTime.tv_nsec,
          (long) sleepStart.tv_sec, (long) sleepStart.tv_nsec,
          (long) currentTime.tv_sec, (long) currentTime.tv_nsec,
          sleepTime, wakeDelay);*/
          
        // Debug Short Print with respect to target sleep = 1 sec. = 1000000000 ns
        long debugTargetDelay=sleepTime-1000000000;
        printf("sleep=%-ld delay=%-ld targetdelay=%-ld\n",    
          sleepTime, wakeDelay, debugTargetDelay);    
    }
  }
}


int main(int argc, char*argv[])
{
  tickThread();
}

