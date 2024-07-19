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
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "kfs.h"

#define KFS_SERVER_CONF                            "/etc/kfs/kfs.conf"

#define OPT_HELP                                   0x01
#define OPT_VERBOSE                                0x02
#define OPT_NO_DETACH                              0x04
#define OPT_CONF                                   0x08


typedef struct{
    char conf_file[KFS_FILENAME_LEN];
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


int parse_opts( int argc, char **argv, options_t *options){
    char opc[] = "c:nvh";
    int opt, flags = 0;

    memset( (void *) options, 0, sizeof( options_t));

    if( argc == 0){
        fprintf(stderr, "ERROR: invalid number of arguments.\n");
        display_help();
        exit( EXIT_FAILURE);
    }      

    
    while ((opt = getopt(argc, argv, opc )) != -1) {
        switch (opt) {
            case 'c':
                flags |= OPT_CONF;
                strcpy( options->conf_file, optarg);
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
        snprintf( options->conf_file, KFS_FILENAME_LEN, "%s", KFS_SERVER_CONF);
        flags |= OPT_CONF;
    }

    options->flags = flags;


    return(0);
}


int main( int argc, char **argv){
    int rc;
    options_t options;
    kfs_config_t config;

    rc = parse_opts( argc, argv, &options);
    if( rc < 0){
        return( rc);
    }

    rc = kfs_config_read( options.conf_file, &config);
    if( rc < 0){
        return( rc);
    }


    kfs_config_display( &config);

    rc = kfs_open( &config);
    if( rc < 0) goto quit;

    kfs_superblock_display();
    rc = kfs_superblock_update();
    if( rc < 0) goto quit;

    rc = kfs_superblock_close();
    if( rc < 0) goto quit;

quit:;
    return rc;

}

