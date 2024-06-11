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


#define FILEDEV_LEN                                250
typedef struct{
    char kfs_file[FILEDEV_LEN];
    int cache_page_len; 
    int cache_ino_len; 
    int cache_path_len;
    int cache_graph_len; 
    int threads_pool; 
    int max_clients;
}config_t; 



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




int read_config( char *filename, config_t *conf){
    char *s, buff[256];
    char delim[] = " =";
#define KVLEN                                      80    
    char key[KVLEN], value[KVLEN];


    memset( (void *) conf, 0, sizeof( config_t));
    FILE *fp = fopen( filename, "r");
    if (fp == NULL){
        TRACE_ERRNO("Could not open %s", filename);
        display_help();
        exit( -1);
    }

    /* Read next line */
    while ((s = fgets(buff, sizeof( buff), fp)) != NULL){
        /* Skip blank lines and comments */
        if ( buff[0] == '\n'){
            continue;
        }

        trim( buff);
        if ( buff[0] == '#'){
            continue;
        }

        /* Parse name/value pair from line */
        s = strtok(buff, delim);
        if ( s == NULL){
            continue;
        }
        strncpy( key, s, KVLEN);

        s = strtok( NULL, delim);
        if ( s == NULL){
            continue;
        }

        strncpy (value, s, KVLEN);

        /* Copy into correct entry in parameters struct */
        if ( strcmp( key, "kfs_file")==0){
            strncpy( conf->kfs_file, value, KVLEN);
        }else if ( strcmp( key, "cache_page_len")==0){
            conf->cache_page_len = atoi( value);
        }else if ( strcmp( key, "cache_ino_len")==0){
            conf->cache_ino_len = atoi( value);
        }else if ( strcmp( key, "cache_path_len")==0){
            conf->cache_path_len = atoi( value);
        }else if ( strcmp( key, "cache_graph_len")==0){
            conf->cache_graph_len = atoi( value);
        }else if ( strcmp( key, "threads_pool")==0){
            conf->threads_pool = atoi( value);
        }else if ( strcmp( key, "max_clients")==0){
            conf->max_clients = atoi( value);
        }else{
            printf("%s/%s: Unknown name/value pair!\n", key, value);
            return( -1);
        }
    }
    /* Close file */
    fclose (fp);
    return(0);
}


void display_config( config_t *conf){
    printf("Conf: \n");
    printf("    kfs_file=<%s>\n", conf->kfs_file);
    printf("    cache_page_len=%d\n", conf->cache_page_len);
    printf("    cache_ino_len=%d\n", conf->cache_ino_len);   
    printf("    cache_path_len=%d\n", conf->cache_path_len);
    printf("    max_clients=%d\n", conf->max_clients);
    printf("    cache_graph_len=%d\n", conf->cache_graph_len);
    printf("    threads_pool=%d\n", conf->threads_pool);
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
        snprintf( options->conf_file, FILENAME_LEN, "%s", KFS_SERVER_CONF);
        flags |= OPT_CONF;
    }

    options->flags = flags;


    return(0);
}

int main( int argc, char **argv){
    int rc;
    options_t options;
    config_t config;

    rc = parse_opts( argc, argv, &options);
    rc = read_config( options.conf_file, &config);

    display_config( &config);

    rc = kfs_open( &config );



    return(0);
}

