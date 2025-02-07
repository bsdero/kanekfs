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
#include "kfs_config.h"
#include "utils.h"

int kfs_config_read( char *filename, kfs_config_t *conf){
    char *s, buff[KFS_FILENAME_LEN];
    char delim[] = " =";
#define KVLEN                                      KFS_FILENAME_LEN   
    char key[KVLEN], value[KVLEN];


    memset( (void *) conf, 0, sizeof( kfs_config_t));
    FILE *fp = fopen( filename, "r");
    if (fp == NULL){
        TRACE_ERRNO("Could not open %s", filename);
        return( -1);
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
        }else if( strcmp( key, "pid_file")==0){
            strncpy( conf->pid_file, value, KVLEN);
        }else if( strcmp( key, "sock_file")==0){
            strncpy( conf->sock_file, value, KVLEN);
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
        }else if ( strcmp( key, "sock_buffer_size")==0){
            conf->sock_buffer_size = atoi( value);
        }else if ( strcmp( key, "root_super_inode")==0){
            conf->root_super_inode = atoi( value);
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


void kfs_config_display( kfs_config_t *conf){
    printf("Conf: \n");
    printf("    kfs_file='%s'\n", conf->kfs_file);
    printf("    sock_file='%s'\n", conf->sock_file);
    printf("    cache_page_len=%d\n", conf->cache_page_len);
    printf("    pid_file='%s'\n", conf->pid_file);
    printf("    cache_ino_len=%d\n", conf->cache_ino_len);   
    printf("    cache_path_len=%d\n", conf->cache_path_len);
    printf("    max_clients=%d\n", conf->max_clients);
    printf("    cache_graph_len=%d\n", conf->cache_graph_len);
    printf("    threads_pool=%d\n", conf->threads_pool);
    printf("    sock_buffer_size=%d\n", conf->sock_buffer_size);
    printf("    root_super_inode=%d\n", conf->root_super_inode);
}

