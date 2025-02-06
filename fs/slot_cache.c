#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <sys/time.h>
#include "trace.h"
#include "kfs_page_cache.h"
#include "kfs_io.h"
#include "kfs_slot_cache.h"


/* if flags == 0, it will exit when (cache->ca_flags == 0).
 * if flags != 0, it will exit when (cache->ca_flags & flags == 0)
 * timeout are seconds to exit. If timeout reached, the return code will
 * be != 0.
 *
 */
int kfs_slotcache_flags_wait( slotcache_t *cache, 
                              uint32_t flags, 
                              int timeout_secs){
    struct timespec start_time, sleep_time, current_time, diff_time;
    int rc = 0;


    clock_gettime( CLOCK_MONOTONIC, &start_time);
    current_time = start_time;
    while(1){
        sleep_time = current_time;
        sleep_time.tv_nsec += NANOSECS_DELAY;
        if( sleep_time.tv_nsec >= MILISECS_PER_SECOND){
            sleep_time.tv_nsec -= MILISECS_PER_SECOND;
            sleep_time.tv_sec++;
        }
        clock_nanosleep( CLOCK_MONOTONIC, TIMER_ABSTIME, &sleep_time, NULL);
        clock_gettime( CLOCK_MONOTONIC, &current_time);

        timespecsub( &current_time, &start_time, &diff_time);
        if( diff_time.tv_sec >= timeout_secs){
            rc = 1;
            break;
        }

        if( ( flags == 0 && cache->sc_flags == 0) || 
            ( flags != 0 && ( ( cache->sc_flags & flags) == flags)) ){
            rc = 0;
            break;
        }
    }

    return( rc);
}


int kfs_slotcache_element_flags_wait( slotcache_element_t *el, 
                                       uint32_t flags, 
                                       int timeout_secs){

    struct timespec start_time, sleep_time, current_time, diff_time;
    int rc = 0;


    clock_gettime( CLOCK_MONOTONIC, &start_time);
    current_time = start_time;
    while(1){
        sleep_time = current_time;
        sleep_time.tv_nsec += NANOSECS_DELAY;
        if( sleep_time.tv_nsec >= MILISECS_PER_SECOND){
            sleep_time.tv_nsec -= MILISECS_PER_SECOND;
            sleep_time.tv_sec++;
        }
        clock_nanosleep( CLOCK_MONOTONIC, TIMER_ABSTIME, &sleep_time, NULL);
        clock_gettime( CLOCK_MONOTONIC, &current_time);

        timespecsub( &current_time, &start_time, &diff_time);
        if( diff_time.tv_sec >= timeout_secs){
            rc = 1;
            break;
        }


        pthread_mutex_lock( &el->sce_mutex);

        if( ( flags == 0 && el->sce_flags == 0) || 
            ( flags != 0 && ( ( el->sce_flags & flags) == flags)) ){
            rc = 0;
        }
        pthread_mutex_unlock( &el->sce_mutex);

        if( rc == 0){
            break;
        }
    }

    return( rc);
}


void *kfs_slotcache_loop_thread( void *cache_p){
    slotcache_t *cache = ( slotcache_t *) cache_p;
    struct timespec remaining, request;
    int rc, i;
    slotcache_element_t *el;
    uint32_t flags;
    uint64_t nsec = 100000000L; /*     1/10th of second */

    TRACE("start");
    cache->sc_tid = pthread_self();
    cache->sc_flags |= KFS_CACHE_ACTIVE;
    request.tv_sec = 0;
    request.tv_nsec = nsec;


    do{
        if( cache->sc_flags & KFS_CACHE_PAUSE_LOOP){
            request.tv_nsec = 0;
            request.tv_sec = 1;  /* pause for one second */
            nanosleep( &request, &remaining);
            request.tv_sec = 0;
            request.tv_nsec = nsec;
            continue;
        }
 
        pthread_mutex_lock( &cache->sc_mutex);
        cache->sc_flags |= KFS_CACHE_ON_LOOP;
        for( i = 0; i < cache->sc_elements_capacity; i++){
            el = &cache->sc_elements[i];
            flags = el->sce_flags;

            if((flags & KFS_CACHE_ND_ACTIVE) == 0){
                continue;
            }

            rc = pthread_mutex_trylock( &el->sce_mutex);
            if( rc != 0){
                continue;
            }

            if( (flags & KFS_CACHE_ND_DIRTY) != 0){
                /* WRITE SLOT INDEX AND SLOT EXTENT HERE */
                if( rc != 0){
                    TRACE_ERR("slot write error");
                }else{
                    el->sce_flags &= ~KFS_CACHE_ND_DIRTY;
                    el->sce_flags |= KFS_CACHE_ND_CLEAN;
                }
                TRACE("flushed out");
            }
            if( (flags & KFS_CACHE_ND_EVICT) != 0){
                /* EVICT FREE MEMORY OF SLOT HERE */

                cache->sc_elements_in_use--;
                el->sce_flags = 0;
                pthread_mutex_unlock( &el->sce_mutex);
                pthread_mutex_destroy( &el->sce_mutex);
            }else{
                pthread_mutex_unlock( &el->sce_mutex);
            }
        }

        cache->sc_flags &= ~KFS_CACHE_ON_LOOP;
        if( (cache->sc_flags & KFS_CACHE_EXIT_LOOP) != 0){
            cache->sc_flags |= KFS_CACHE_EVICTED;
        }
        pthread_mutex_unlock( &cache->sc_mutex);
        nanosleep( &request, &remaining);

    }while( (cache->sc_flags & KFS_CACHE_EVICTED) == 0);


    cache->sc_flags = ( KFS_CACHE_EXIT_LOOP | KFS_CACHE_EVICTED);

    TRACE("end");
    pthread_exit( NULL);
}


slotcache_element_t *kfs_slotcache_element_get( slotcache_t *cache, 
                                        uint64_t addr, 
                                        int numblocks, 
                                        uint32_t flags,
                                        void *(*func)(void *)   ){
    int i, rc;
    slotcache_element_t *el;
    
    TRACE("start");

    el = NULL;

    pthread_mutex_lock( &cache->sc_mutex);

    TRACE("in mutex");
    /* search an empty cache slot */
    for( i = 0; i < cache->sc_elements_capacity; i++){
        el = &cache->sc_elements[i];

        if( (el->sce_flags & KFS_CACHE_ND_ACTIVE) != 0){
            continue;
        }

        break;
    }

    if( el != NULL){
        /* found a cache slot, fill in all the required data */



       if (pthread_mutex_init( &el->sce_mutex, NULL) != 0) { 
            TRACE_ERR("mutex init has failed");
            el = NULL;
            goto exit0;
        } 




        el->sce_flags = flags | KFS_CACHE_ND_ACTIVE | KFS_CACHE_ND_CLEAN;
 
        cache->sc_elements_in_use++;
    }else{
        TRACE_ERR("cache is full, can not map other extent");
    }
exit0:

    pthread_mutex_unlock( &cache->sc_mutex);
    
    TRACE("end");
    return( el);
}


slotcache_t *kfs_slotcache_alloc( sb_t *sb, int num_elems){
    slotcache_t *slotcache;
    void *p;

    TRACE("start");

    
    slotcache = malloc( sizeof( slotcache_t ));
    if( slotcache == NULL){
        TRACE_ERR("Error in malloc()"); 
        goto exit0;
    }

    memset( (void *) slotcache, 0, sizeof( slotcache_t));

    p = malloc( sizeof( slotcache_element_t) * num_elems);
    if( p == NULL){
        TRACE_ERR("Error in malloc()");
        goto exit1;
    }

    memset( p, 0, sizeof( slotcache_element_t) * num_elems);
   
    slotcache->sc_elements = p;
    slotcache->sc_elements_capacity = num_elems;
    slotcache->sc_elements_in_use = 0;
    slotcache->sc_sb = sb;

    if (pthread_mutex_init(&slotcache->sc_mutex, NULL) != 0) { 
        TRACE_ERR("mutex init has failed"); 
        goto exit1; 
    } 

    slotcache->sc_flags = KFS_CACHE_READY;
    goto exit0; 


exit1:
    free( slotcache);
    slotcache = NULL;

exit0:
    TRACE("end");

    return( slotcache);
}



int kfs_slotcache_start_thread( slotcache_t *cache){
    int rc;

    TRACE("start");

    if( (cache->sc_flags & KFS_CACHE_READY) == 0){
        TRACE_ERR("cache is not ready, abort");
        return(-1);
    }

    rc = pthread_create( &cache->sc_thread, 
                         NULL, 
                         kfs_slotcache_loop_thread,
                         cache);
    if( rc != 0){
        TRACE_ERR("pthread_create() failed");
        cache->sc_thread = pthread_self();
        return( -1);
    }

    TRACE("end");
    return(0);
}


int kfs_slotcache_destroy( slotcache_t *cache){
    int i, rc = 0;
    slotcache_element_t *el;
    void *ret;
    TRACE("start");
    if( (cache->sc_flags & KFS_CACHE_READY) == 0){
        TRACE_ERR("cache is not ready, abort");
        return(-1);
    }

    pthread_mutex_lock( &cache->sc_mutex);
    TRACE("in mutex");
    for( i = 0; i < cache->sc_elements_capacity; i++){
        el = &cache->sc_elements[i];

        if((el->sce_flags & KFS_CACHE_ND_ACTIVE) == 0){
            continue;
        }
        /* MARK SLOT AS DIRTY HERE */
        if( rc != 0){
            TRACE_ERR("Cache element not active");
        }
    }
    
    cache->sc_flags |= KFS_CACHE_EXIT_LOOP;    
    pthread_mutex_unlock( &cache->sc_mutex);

    /* wait for the thread loop to run and remove/sync all the stuff */
    rc = kfs_slotcache_flags_wait( cache, 
                                 KFS_CACHE_EXIT_LOOP | KFS_CACHE_EVICTED, 
                                 10);
    if( rc != 0){
        TRACE_ERR("timeout waiting for flags = 0");
    }

    /* if we are here the thread should be finished already. */
    if( pthread_equal( cache->sc_thread, cache->sc_tid)){
        pthread_join( cache->sc_thread, &ret);
    }
    
    pthread_mutex_destroy( &cache->sc_mutex);

    free( cache->sc_elements);
    free( cache);
    TRACE("end");

    return(0);
}



