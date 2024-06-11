#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include "kfs.h"
#include "map.h"
#include "kfs_io.h"


int main( int argc, char **argv){
    char *filename;
    int rc;

    if( argc != 2 ){
        printf("ERROR: Incorrect arguments number\n");
        printf("Usage: \n");
        printf("    kfs_info <filename>\n");
        printf("\nkfs_info displays kfs filesystem information\n");
        printf("---------------------------------------------------------\n");
        return( -1);
    }
    
    filename = argv[1];

    rc = kfs_verify( filename);
    return( rc);
}

