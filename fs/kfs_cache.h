#ifndef _CACHE_H_
#define _CACHE_H_

#include <sys/time.h>
#include <pthread.h>
#include <stdint.h>
#include "trace.h"


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
#define CACHE_EL_ACTIVE        0x0001 /* write flag, set this if this 
                                         element has valid data. */

#define CACHE_EL_DIRTY         0x0002 /* read flag, enabled if element
                                         is dirty, and its sync is 
                                         pending. This element should be
                                         removed from the loaded list and
                                         placed into dirty list */

#define CACHE_EL_EVICT         0x0004 /* read flag, enabled if element
                                         will be removed and free its 
                                         memory */

#define CACHE_EL_PINNED        0x0008 /* write flag, set this for
                                         not remove this from cache on 
                                         LRU removal */

#define CACHE_EL_CLEAN         0x0010 /* this flag should be enabled only
                                         if the node is active and NOT
                                         dirty */
#define CACHE_EL_MASK          0x00ff 



/* generic flags for cache thread loop */
#define CACHE_READY            0x0001 /* read flag. If enabled,
                                         the cache is ready for run */
#define CACHE_ACTIVE           0x0002 /* read flag. if enabled, 
                                       * processing loops may run any 
                                       * time */
#define CACHE_SYNC             0x0004 /* write flag, flush all the dirty 
                                       * elements, clean all except pinned
                                       * elements. */

#define CACHE_FLUSH            0x0008 /* set this for flush all the
                                         buffers in cache */

#define CACHE_PAUSE_LOOP       0x0010 /* set this for pause the loops,
                                         clear it for unpause */

#define CACHE_EXIT_LOOP        0x0020 /* set this for exit the loop
                                         flush out buffers, and 
                                         evict all */

#define CACHE_ON_LOOP          0x0040 /* read flag, this signals the loop
                                         is running */

#define CACHE_EVICTED          0x0080   /* the list has been evicted */


/* each element in cache.
 *
 * */
typedef struct{
    uint64_t ce_id;  /* id element cache */
    pthread_mutex_t ce_mutex;  /* mutex */
    uint64_t ce_flags;
    uint64_t ce_access_count;
    void *ce_owner_cache; /* pointer to the owner cache structure */
}cache_element_t;

/* elements list in cache */ 
typedef struct{

    /* pointer to an array of pointers to the cache elements. */
    cache_element_t *ca_elements_ptr;
    
    /* number of elements in use and total */
    uint32_t ca_elements_in_use;
    uint32_t ca_elements_capacity;

    /* mutex for this specific cache */
    pthread_mutex_t ca_mutex;
    uint16_t ca_flags;
    pthread_t ca_thread; /* thread */
    pthread_t ca_tid; /* thread id */

    /* pointer to functions which will be executed when something
     * happens to a cache element. The argument should be a pointer
     * to the cache element. */
    void *(*ca_on_map_callback)(void *);    /* on map */
    void *(*ca_on_evict_callback)(void *);  /* on evict */
    void *(*ca_on_flush_callback)(void *);  /* on flush */
 
    /* pointer to extra data for this cache operation */
    void *ca_data;
}cache_t;


/* cache functions */


/* create and init a cache_t structure */
cache_t *cache_alloc( int elements_capacity, 
                      void *data,
                      void *(*on_map)(void *),
                      void *(*on_evict)(void *),
                      void *(*on_flush)(void *));

/* start the cache thread */
int cache_run( cache_t *cache);

/* enable the specified flag. Only write flags are allowed */
int cache_set_flag( cache_t *cache, uint16_t flag_to_enable);
int cache_destroy( cache_t *cache);
int cache_sync( cache_t *cache);
int cache_pause( cache_t *cache);
int cache_wait_for_flag( cache_t *cache, uint32_t flags, int timeout_secs);
int cache_stop( cache_t *cache);
cache_element_t *cache_lookup( cache_t *cache, uint64_t key);
 

cache_element_t *cache_element_map( cache_t *cache, size_t byte_size);
int cache_element_unmap( cache_element_t *ce);
int cache_element_flush( cache_element_t *ce);
int cache_element_set_flag( cache_element_t *ce);
int cache_element_pin( cache_element_t *ce);
int cache_element_set_active( cache_element_t *ce);
int cache_element_un_pin( cache_element_t *ce);
int cache_element_wait_for_flag( cache_element_t *ce, 
                                     uint32_t flags, 
                                     int timeout_secs);

#endif

