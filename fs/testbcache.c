#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "dumphex.h"
#include "kfs_cache.h"


int main( int argc, char **argv){
    cache_t cache;
    int rc, fd;
    char *filename;
    cache_element_t *el, *el2;
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
   
    rc = kfs_cache_alloc( &cache, fd, 32, 10000);
    if( rc != 0){
        TRACE_ERR("Issues in kfs_cache_alloc()");
        close( fd);
        return( -1);
    }
    

    rc = kfs_cache_start_thread( &cache);
    sleep(1);

    el = kfs_cache_element_map( &cache, 0, 1, 0, NULL);
    if( el == NULL){
        TRACE_ERR("Issues in kfs_cache_alloc()");
        close( fd);
        return( -1);
    }
    sleep(1);

    strcpy( (char *) el->ce_mem_ptr, s1);
    kfs_cache_element_mark_dirty( el);
    sleep(1);

    el2 = kfs_cache_element_map( &cache, 10, 1, 0, NULL);
    if( el2 == NULL){
        TRACE_ERR("Issues in kfs_cache_alloc()");
        close( fd);
        return( -1);
    }
    sleep(1);
    strcpy( (char *) el->ce_mem_ptr, s2);
    kfs_cache_element_mark_dirty( el);
    strcpy( (char *) el2->ce_mem_ptr, s1);
    kfs_cache_element_mark_dirty( el2);
    sleep(2);





    rc = kfs_cache_destroy( &cache);
    close( fd);

    
    return( rc);
}

