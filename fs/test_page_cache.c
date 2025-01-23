#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "dumphex.h"
#include "kfs_page_cache.h"


int main( int argc, char **argv){
    pgcache_t *cache;
    int rc, fd;
    char *filename;
    pgcache_element_t *el, *el2;
    char s1[] = "dabale arroz a la zorra el abad";
    char s2[] = "was it a car or a cat i saw";
    if( argc != 2){
        printf("Usage: testbcache <file>\n");
        return(-1);
    }

    printf("argc=%d, argv=['%s', '%s' ]\n", argc, argv[0], argv[1]);
    filename = argv[1];
    fd = open( filename, O_RDWR );
    if( fd < 0){
        TRACE_ERR( "Could not open file '%s'\n", filename);
        return( -1);
    }
       
    cache = kfs_pgcache_alloc( fd, 32);
    if( cache == NULL){
        TRACE_ERR("Issues in kfs_pgcache_alloc()");
        close( fd);
        return( -1);
    }
        

    rc = kfs_pgcache_start_thread( cache);
    if( rc != 0){
        TRACE_ERR("Issues in kfs_pgcache_start_thread()");
        close( fd);
        return( -1);
    }
        
    rc = kfs_pgcache_flags_wait( cache, KFS_CACHE_ACTIVE, 10);
    if( rc != 0){
        TRACE_ERR("timeout waiting for KFS_CACHE_ACTIVE");
        close( fd);
        return( -1);
    }


    el = kfs_pgcache_element_map( cache, 0, 1, 0, NULL);
    if( el == NULL){
        TRACE_ERR("Issues in kfs_pgcache_alloc()");
        close( fd);
        return( -1);
    }


    rc = kfs_pgcache_element_flags_wait( el, 
                                         KFS_CACHE_ND_ACTIVE, 
                                         10);
    if( rc != 0){
        TRACE_ERR("timeout waiting for KFS_CACHE_ND_ACTIVE");
        close( fd);
        return( -1);
    }

    strcpy( (char *) el->ce_mem_ptr, s1);
    kfs_pgcache_element_mark_dirty( el);
    rc = kfs_pgcache_element_flags_wait( el, 
                                         KFS_CACHE_ND_CLEAN, 
                                         10);
    
    if( rc != 0){
        TRACE_ERR("timeout waiting for KFS_CACHE_ND_CLEAN");
        close( fd);
        return( -1);
    }


    el2 = kfs_pgcache_element_map( cache, 10, 1, 0, NULL);
    if( el2 == NULL){
        TRACE_ERR("Issues in kfs_pgcache_alloc()");
        close( fd);
        return( -1);
    }
    rc = kfs_pgcache_element_flags_wait( el2, 
                                            KFS_CACHE_ND_ACTIVE, 
                                            10);
    if( rc != 0){
        TRACE_ERR("timeout waiting for KFS_CACHE_ND_ACTIVE");
        close( fd);
        return( -1);
    }

    strcpy( (char *) el->ce_mem_ptr, s2);
    kfs_pgcache_element_mark_dirty( el);
    strcpy( (char *) el2->ce_mem_ptr, s1);
    kfs_pgcache_element_mark_dirty( el2);

    rc = kfs_pgcache_element_flags_wait( el, KFS_CACHE_ND_CLEAN, 10);
    if( rc != 0){
        TRACE_ERR("timeout waiting for KFS_CACHE_ND_CLEAN");
        close( fd);
        return( -1);
    }

    rc = kfs_pgcache_element_flags_wait( el2, KFS_CACHE_ND_CLEAN, 10);
    if( rc != 0){
        TRACE_ERR("timeout waiting for KFS_CACHE_ND_CLEAN");
        close( fd);
        return( -1);
    }


    rc = kfs_pgcache_destroy( cache);
    close( fd);

    return( rc);
}


