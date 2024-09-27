#ifndef _CACHE_H_
#define _CACHE_H_


#include <sys/time.h>
#include <stdint.h>


/* 1000000000 nanosecs == 1 second. */
#define NANOSECS_DELAY                   100000000ul
#define MILISECS_PER_SECOND              1000000000ul


#ifndef timespecsub
#define	timespecsub(tsp, usp, vsp)					\
	do {								\
		(vsp)->tv_sec = (tsp)->tv_sec - (usp)->tv_sec;		\
		(vsp)->tv_nsec = (tsp)->tv_nsec - (usp)->tv_nsec;	\
		if ((vsp)->tv_nsec < 0) {				\
			(vsp)->tv_sec--;				\
			(vsp)->tv_nsec += 1000000000L;			\
		}							\
	} while (0)
#endif



/* generic flags for cache elements */
#define KFS_CACHE_ND_ACTIVE        0x0001 /* has this element valid data? */
#define KFS_CACHE_ND_DIRTY         0x0002 /* dirty? should we sync? */
#define KFS_CACHE_ND_EVICT         0x0004 /* remove and free memory */
#define KFS_CACHE_ND_PINNED        0x0008 /* dont remove this from cache */
#define KFS_CACHE_ND_FORCE_MAP     0x0010 /* evict LRU cache element and 
                                                put this one on place */
#define KFS_CACHE_ND_CLEAN         0x0020 /* this flag should be enabled only
                                             if the node is active and NOT
                                             dirty */
#define KFS_CACHE_ND_MASK          0x00ff 


/* generic flags for cache thread loop */
#define KFS_CACHE_READY            0x01 /* read flag. If enabled,
                                           the cache is ready for run */
#define KFS_CACHE_ACTIVE           0x02 /* read flag. if enabled, 
                                         * processing loop may run any 
                                         * time */
#define KFS_CACHE_ON_LOOP          0x04 /* read flag, if enabled 
                                         * the processing loop is 
                                         * running */
#define KFS_CACHE_FLUSH            0x08 /* set this for flush all the
                                           buffers in cache */
#define KFS_CACHE_PAUSE_LOOP       0x10 /* set this for pause the loop,
                                           clear it for un pause */

#define KFS_CACHE_EXIT_LOOP        0x20 /* set this for exit the loop
                                           flush out buffers, and 
                                           evict all */
#define KFS_CACHE_EVICTED          0x40 /* if enabled, eveything has 
                                           been evicted, out now */
/* element in cache */
typedef struct{
    uint64_t ca_id;  /* id element cache */
    pthread_mutex_t ca_mutex;  /* 
    void *(*ca_on_map_callback)(void *);
    void *(*ca_on_evict_callback)(void *);
    void *(*ca_on_flush_callback)(void *);
    uint64_t ca_flags;
    uint64_t ca_access_count;
}cache_element_t;

typedef struct{
    void **ptr_elements;
    
    /* number of elements in use and total */
    uint32_t ca_elements_in_use;
    uint32_t ca_elements_capacity;

    pthread_mutex_t ca_mutex; 
    uint16_t ca_flags;
    int ca_fd;
    pthread_t ca_thread; /* thread */
    pthread_t ca_tid; /* thread id */

#endif

