#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <sys/time.h>
#include <limits.h>
#include "trace.h"
#include "cache.h"



void *cache_thread( void *cache_p){
    cache_t *cache = ( cache_t *) cache_p;
    struct timespec remaining, request;
    int rc, i;
    void *arg_res; 
    cache_element_t *el;
    uint32_t flags;
    uint64_t nsec = 100000000L; /*     1/10th of second */

    TRACE("start");
    cache->ca_tid = pthread_self();
    cache->ca_flags |= CACHE_ACTIVE;
    request.tv_sec = 0;
    request.tv_nsec = nsec;


    do{
        if( cache->ca_flags & CACHE_PAUSE){
            request.tv_nsec = 0;
            request.tv_sec = 1;  /* pause for one second */
            nanosleep( &request, &remaining);
            request.tv_sec = 0;
            request.tv_nsec = nsec;
            continue;
        }
 
        pthread_mutex_lock( &cache->ca_mutex);

        /* if any element exist */
        if( cache->ca_elements_in_use > 0){
            /* traverse the array */
            cache->ca_flags |= CACHE_ON_LOOP;
            for( i = 0; i < cache->ca_elements_capacity; i++){
                el = cache->ca_elements_ptr[i];
                if( el == NULL){
                    continue;
                }

                flags = el->ce_flags;

                if((flags & CACHE_EL_ACTIVE) == 0){
                    continue;
                }

                rc = pthread_mutex_trylock( &el->ce_mutex);
                if( rc != 0){
                    continue;
                }

                if( (flags & CACHE_EL_DIRTY) != 0){
                    el->ce_access_count++;
                    if( cache->ca_on_flush_callback != NULL){
                        arg_res = (void *) &rc;
                        arg_res = cache->ca_on_flush_callback( (void *) el);
                        if( rc != 0){
                            TRACE_ERR( "error in on_flush_callback, rc=%d",
                                       *((int *)arg_res) );
                        }

                    }else{
                        TRACE("WARNING: no on_flush_callback method!!");
                    }
                    
                    el->ce_flags &= ~CACHE_EL_DIRTY;
                    el->ce_flags |= CACHE_EL_CLEAN;
                    TRACE("flushed out");
                }

                if( (flags & CACHE_EL_EVICT) != 0){
                    if( cache->ca_on_evict_callback != NULL){
                        arg_res = (void *) &rc; 
                        arg_res = cache->ca_on_evict_callback( ( void *) el );
                        if( rc != 0){
                            TRACE_ERR("error in on_evict_callback, rc=%d",
                                      *((int *)arg_res) );
                        }
                    }

                    cache->ca_elements_in_use--;
                    el->ce_flags = 0;
                    pthread_mutex_unlock( &el->ce_mutex);
                    pthread_mutex_destroy( &el->ce_mutex);

                    free( el);
                    cache->ca_elements_ptr[i] = NULL;
                }else{
                    pthread_mutex_unlock( &el->ce_mutex);
                }
            }
        }
        cache->ca_flags &= ~CACHE_ON_LOOP;
        if( (cache->ca_flags & CACHE_EXIT) != 0){
            cache->ca_flags |= CACHE_EVICTED;
        }
        if( (cache->ca_flags & CACHE_SET_LOOP_DONE) != 0){
            cache->ca_flags &= ~CACHE_SET_LOOP_DONE;
            cache->ca_flags |= CACHE_LOOP_DONE;
        }
        pthread_mutex_unlock( &cache->ca_mutex);
        nanosleep( &request, &remaining);

    }while( (cache->ca_flags & CACHE_EVICTED) == 0);


    cache->ca_flags = ( CACHE_EXIT | CACHE_EVICTED);

    TRACE("end");
    pthread_exit( NULL);
}



cache_t *cache_alloc( int elements_capacity, 
                          void *data,
                          void *(*on_map)(void *),
                          void *(*on_evict)(void *),
                          void *(*on_flush)(void *)){
    cache_t *cache;
    void *p;

    TRACE("start");

    
    cache = malloc( sizeof( cache_t ));
    if( cache == NULL){
        TRACE_ERR("Error in malloc()"); 
        goto exit0;
    }
    memset( (void *) cache, 0, sizeof( cache_t));

    /* reserve an array of pointers */
    p = malloc( sizeof( cache_element_t *) * elements_capacity);
    if( p == NULL){
        TRACE_ERR("Error in malloc()");
        goto exit1;
    }
    memset( p, 0, sizeof( cache_element_t *) * elements_capacity);
   
    cache->ca_elements_ptr = p;
    cache->ca_elements_capacity = elements_capacity;
    cache->ca_elements_in_use = 0;
    cache->ca_data = data;
    cache->ca_on_map_callback = on_map;
    cache->ca_on_evict_callback = on_evict;
    cache->ca_on_flush_callback = on_flush;

    if (pthread_mutex_init(&cache->ca_mutex, NULL) != 0) { 
        TRACE_ERR("mutex init has failed"); 
        goto exit1; 
    } 

    /* the cache thread is ready to run */
    cache->ca_flags = CACHE_READY;
    goto exit0; 


exit1:
    free( cache);
    cache = NULL;

exit0:
    TRACE("end");

    return( cache);
}


int cache_run( cache_t *cache){
    int rc;

    TRACE("start");

    if( (cache->ca_flags & CACHE_READY) == 0){
        TRACE_ERR("cache is not ready, abort");
        return(-1);
    }

    rc = pthread_create( &cache->ca_thread, 
                         NULL, 
                         cache_thread,
                         cache);
    if( rc != 0){
        TRACE_ERR("pthread_create() failed");
        cache->ca_thread = pthread_self();
        return( -1);
    }

    TRACE("end");
    return(0);
}


void cache_element_evict( cache_t *cache, int subin){
/* a cache mutex on is assumed!!! */
    int rc, *arg_res;
    cache_element_t *el;
    uint32_t flags;


    el = cache->ca_elements_ptr[subin];
    rc = pthread_mutex_lock( &el->ce_mutex);

   
    flags = el->ce_flags;
    if( (flags & CACHE_EL_DIRTY) != 0){
        el->ce_access_count++;
        if( cache->ca_on_flush_callback != NULL){
            arg_res = &rc;
            arg_res = (void *) cache->ca_on_flush_callback( (void *) el);
            if( rc != 0){
                TRACE_ERR( "error in on_flush_callback, rc=%d",
                           *((int *)arg_res) );
            }

        }else{
            TRACE("WARNING: no on_flush_callback method!!");
        }
                    
        el->ce_flags &= ~CACHE_EL_DIRTY;
        el->ce_flags |= CACHE_EL_CLEAN;
        TRACE("flushed out");
    }

    if( cache->ca_on_evict_callback != NULL){
        arg_res = (void *) &rc; 
        arg_res = cache->ca_on_evict_callback( ( void *) el );
        if( rc != 0){
            TRACE_ERR( "error in on_evict_callback, rc=%d",
                       *((int *)arg_res) );
        }
    }

    cache->ca_elements_in_use--;
    el->ce_flags = 0;
    pthread_mutex_unlock( &el->ce_mutex);
    pthread_mutex_destroy( &el->ce_mutex);

    free( el);
    cache->ca_elements_ptr[subin] = NULL;
}

int cache_set_flags(  cache_t *cache, uint32_t flag_to_enable){
    uint16_t flags_mask = CACHE_SYNC|CACHE_FLUSH|CACHE_PAUSE|CACHE_EXIT;

    flags_mask |= CACHE_SET_LOOP_DONE;

    if( (flag_to_enable & flags_mask) != 0){
        return( -1);
    }
    pthread_mutex_lock( &cache->ca_mutex);
    cache->ca_flags |= flag_to_enable;
    pthread_mutex_unlock( &cache->ca_mutex);
    return( 0); 
}

int cache_clear_loop_done( cache_t *cache){
    pthread_mutex_lock( &cache->ca_mutex);
    cache->ca_flags &= ~CACHE_LOOP_DONE;
    pthread_mutex_unlock( &cache->ca_mutex);
    return( 0); 
}


int cache_destroy( cache_t *cache){
    int rc = 0;
    void *ret;
    TRACE("start");

    cache_sync( cache);
    cache_set_flags( cache, CACHE_EXIT);

    /* wait for the thread loop to run and remove/sync all the stuff */
    rc = cache_wait_for_flags( cache, CACHE_EXIT| CACHE_EVICTED, 10);
    if( rc != 0){
        TRACE_ERR("timeout waiting for flags = 0");
    }

    /* if we are here the thread should be finished already. */
    if( pthread_equal( cache->ca_thread, cache->ca_tid)){
        pthread_join( cache->ca_thread, &ret);
    }
    
    pthread_mutex_destroy( &cache->ca_mutex);

    free( cache->ca_elements_ptr);
    free( cache);
    TRACE("end");

    return(0);
}

int cache_sync( cache_t *cache){
    int i;
    cache_element_t *el;
    TRACE("start");
    if( (cache->ca_flags & CACHE_READY) == 0){
        TRACE_ERR("cache is not ready, abort");
        return(-1);
    }

    pthread_mutex_lock( &cache->ca_mutex);
    for( i = 0; i < cache->ca_elements_capacity; i++){
        el = cache->ca_elements_ptr[i];

        if( el != NULL){
            cache_element_mark_eviction( el);
        }
    }
    pthread_mutex_unlock( &cache->ca_mutex);

    TRACE("end");
    return(0);
}

int cache_pause( cache_t *cache){
    return( cache_set_flags( cache, CACHE_PAUSE));
}


int cache_clear_pause( cache_t *cache){
    pthread_mutex_lock( &cache->ca_mutex);
    cache->ca_flags &= ~CACHE_PAUSE;
    pthread_mutex_unlock( &cache->ca_mutex);
    return( 0); 
}

/* if flags == 0, it will exit when (cache->ca_flags == 0).
 * if flags != 0, it will exit when the flags are active.
 * timeout are seconds to exit. If timeout reached, the return code will
 * be != 0.
 *
 */
int cache_wait_for_flags( cache_t *cache, uint32_t flags, int timeout_secs){
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

        if( ( flags == 0 && cache->ca_flags == 0) || 
            ( flags != 0 && ( ( cache->ca_flags & flags) == flags)) ){
            rc = 0;
            break;
        }
    }

    return( rc);
}



cache_element_t *cache_lookup( cache_t *cache, uint64_t key){
    cache_element_t *el, *ret = NULL;
    int i;

    pthread_mutex_lock( &cache->ca_mutex);
    for( i = 0; i < cache->ca_elements_capacity; i++){
        el = cache->ca_elements_ptr[i];

        if( el != NULL){
            if( el->ce_id == key){
                ret = el;
                el->ce_access_count++;
                break;
            }
        }
    }
    pthread_mutex_unlock( &cache->ca_mutex);
    return( ret);
}



cache_element_t *cache_element_map( cache_t *cache, size_t byte_size){
    int i, min, sub;
    cache_element_t *el;

    
    TRACE("start");

    el = NULL;

    pthread_mutex_lock( &cache->ca_mutex);

    TRACE("in mutex");
    /* search an empty cache slot */
    for( i = 0; i < cache->ca_elements_capacity; i++){
        if( cache->ca_elements_ptr[i] == NULL){
            el = cache->ca_elements_ptr[i];
            sub = i;
            break;
        }
    }


    /* did not found anything!! locate the LRU element and evict */
    if( el == NULL){
        min = INT_MAX;


        for( i = 0; i < cache->ca_elements_capacity; i++){
            el = cache->ca_elements_ptr[i];
            if( el != NULL){
                if( el->ce_access_count < min ){
                    min = el->ce_access_count; 
                    sub = i;
                }
            }
        }

        cache_element_evict( cache, sub);
    }

    el = malloc( sizeof( cache_element_t) + byte_size);
    if( el == NULL){
        TRACE_ERR("malloc error");
        el = NULL;
        goto exit0;
    }

    memset( el, 0, sizeof( cache_element_t) + byte_size);
    if( pthread_mutex_init( &el->ce_mutex, NULL) != 0) { 
        TRACE_ERR("mutex init has failed");
        free( el);
        el = NULL;
        goto exit0;
    } 

    el->ce_flags = CACHE_EL_ACTIVE | CACHE_EL_CLEAN;
    el->ce_access_count = 1;
    el->ce_id = (uint64_t) el; 
    el->ce_owner_cache = cache;
 
    cache->ca_elements_in_use++;
    cache->ca_elements_ptr[sub] = el;
exit0:

    pthread_mutex_unlock( &cache->ca_mutex);
    
    TRACE("end");
    return( el);
}




int cache_element_mark_eviction( cache_element_t *ce){
    TRACE("start");
    if(( ce->ce_flags & CACHE_EL_ACTIVE) == 0){
        TRACE_ERR("cache element not active");
        return(-1);
    }


    pthread_mutex_lock( &ce->ce_mutex);
    ce->ce_flags |= CACHE_EL_EVICT;
    pthread_mutex_unlock( &ce->ce_mutex);
    TRACE("end");
    return(0);
}


int cache_element_mark_dirty( cache_element_t *ce){
    TRACE("start");
    if(( ce->ce_flags & CACHE_EL_ACTIVE) == 0){
        TRACE_ERR("cache element not active");
        return(-1);
    }


    pthread_mutex_lock( &ce->ce_mutex);
    ce->ce_flags |= CACHE_EL_DIRTY;
    ce->ce_flags &= ~CACHE_EL_CLEAN;
    pthread_mutex_unlock( &ce->ce_mutex);
    TRACE("end");
    return(0);
}



int cache_element_wait_for_flags( cache_element_t *el, 
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


        pthread_mutex_lock( &el->ce_mutex);

        if( ( flags == 0 && el->ce_flags == 0) || 
            ( flags != 0 && ( ( el->ce_flags & flags) == flags)) ){
            rc = 0;
        }
        pthread_mutex_unlock( &el->ce_mutex);

        if( rc == 0){
            break;
        }
    }

    return( rc);
}


