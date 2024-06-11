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

#define KFS_SERVER_CONF                            "/etc/kfs/kfs.conf"

#define OPT_HELP                                   0x01
#define OPT_VERBOSE                                0x02
#define OPT_NO_DETACH                              0x04
#define OPT_CONF                                   0x08


#define FILENAME_LEN                               250
typedef struct{
    char conf_file[FILENAME_LEN];
    unsigned int flags;
}options_t; 


int display_help(){
    char *help[]={
        "Usage: kfs_server [options] ",
        "",
        "    Options:",
        "      -c conf_file      Configuration file",
        "      -n                No detach from terminal, logs to stdout",
        "      -v                Verbose mode, more v's, more verbose",
        "      -h                Help",
        "",
        "    kfs_server reads the configuration file and start a KFS user ",
        "    space file server.                                           ",
        "                                                                 ",
        "",       
        "      <bsdero@gmail.com>",
        "",
        "",
        "-------------------------------------------------------------------",
        NULL
    };

    char **s = help;
    while( *s){
        printf( "%s\n", *s++);
    }

    return(0);
}






int main( int argc, char **argv){
    int rc, opt, flags = 0;
    struct stat st;
    char opc[] = "c:nvh";


    memset( (void *) &options, 0, sizeof( options_t));

    if( argc == 0){
        fprintf(stderr, "ERROR: invalid number of arguments.\n");
        display_help();
        exit( EXIT_FAILURE);
    }      

    
    while ((opt = getopt(argc, argv, opc )) != -1) {
        switch (opt) {
            case 'c':
                flags |= OPT_CONF;
                strcpy( options.conf_file, optarg);
                break;
            case 'n':
                flags |= OPT_NO_DETACH;
                break;
            case 'v':
                flags |= OPT_VERBOSE;
                break;
            case 'h':
                flags |= OPT_HELP;
                break;
            default: /* '?' */
                fprintf(stderr, "ERROR: invalid argument.\n");
                display_help();
                exit(EXIT_FAILURE);
        }
    }

    /* if help required, display help and exit, no errors */
    if( flags == OPT_HELP){
        display_help();
        exit( 0);
    }

    if( (flags & OPT_CONF ) == 0){
        snprintf( options.conf_file, FILENAME_LEN, "%s", KFS_SERVER_CONF);
        flags |= OPT_CONF;
    }

    options.flags = flags;
/*
    rc = read_config( options.conf_file);

    rc = validate_config( conf_file);
*/


    return(0);
}

