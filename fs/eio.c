#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include "kfs_mem.h"
#include "eio.h"



int get_bd_size( char *fname, uint64_t *size){
    *size = 0;

#ifdef BLKGETSIZE64
    int fd = open( fname, O_RDONLY);
    if( fd < 0){
        TRACE_ERR("Could not open block device \n");
        return( -1);
    }
    ioctl( fd, BLKGETSIZE64, size);
    close(fd);
    return( 0);
#else
    TRACE_ERR("Operation not supported\n");
    return( -1);
#endif
}



int create_file( char *fname){
    int fd = creat( fname, 0644);
    if( fd < 0){
        TRACE_ERRNO( "Could not create file '%s'", fname);
        return( -1);
    }
    close(fd);
    return(0);
}


char *extent_alloc( int n){
    char *extent;

    extent = malloc( KFS_BLOCKSIZE * n);
    if( extent == NULL){
        TRACE_SYSERR( "malloc error");
        return( NULL);
    }

    /* Fill the whole file with zeroes */
    memset( extent, 0, (KFS_BLOCKSIZE * n) );
    return( extent);
}

int block_read( int fd, char *page, uint64_t addr){
    lseek( fd, addr * KFS_BLOCKSIZE, SEEK_SET);
    read( fd, page, KFS_BLOCKSIZE );
    return(0);
}


int block_write( int fd, char *page, uint64_t addr){
    lseek( fd, addr * KFS_BLOCKSIZE, SEEK_SET);
    write( fd, page, KFS_BLOCKSIZE);

    return(0);
}


int extent_read( int fd, char *extent, uint64_t addr, int block_num){
    lseek( fd, addr * KFS_BLOCKSIZE, SEEK_SET);
    read( fd, extent, KFS_BLOCKSIZE * block_num );
    return(0);
}


int extent_write( int fd, char *extent, uint64_t addr, int block_num){
    lseek( fd, addr * KFS_BLOCKSIZE, SEEK_SET);
    write( fd, extent, KFS_BLOCKSIZE * block_num );

    return(0);
}


