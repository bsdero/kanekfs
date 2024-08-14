#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "dumphex.h"
#include "dict.h"
#include "kfs_cache.h"

#define OPT_F                            0x01
#define OPT_K                            0x02
#define OPT_V                            0x04
#define OPT_T                            0x08
#define OPT_HELP                         0x10

int display_help(){
    char *help[]={
        "Usage: kfs_sbsetmeta <-f file> <-k key> <-v value> [-t datatype]  ",
        "",
        "    kfs_sbsetmeta adds one key-value pair to the slot #0 of the   ",
        "    file system. The slot #0 is exclusive for the superblock.     ",
        "",
        "       <-f file> is a file or a block device with a valid KanekFS ",
        "       filesystem. The metadata will be created in the superblock.",
        "       <-k key> Key of the metadata to add. ",
        "       <-v value> Value of the metadata to add.",
        "       [-t datatype] Data type of the metadata, which can be one  ",
        "                     of the next:                                 ",
        "                     [ int, uint, string, boolean, float ]        ",                            
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


typedef struct{
    char filename[250];
    dict_entry_t entry;
}sbmeta_t;

int main( int argc, char **argv){
    cache_t cache;
    int rc, fd, opt, flags, t;
    cache_element_t *el, *el2;
    char opc[] = "f:k:v:t:ih";
    sbmeta_t meta;
    char val[128];
    char type[128];


    memset( (void *) &meta, 0, sizeof( sbmeta_t));

    if( argc == 0){
        fprintf(stderr, "ERROR: invalid number of arguments.\n");
        display_help();
        exit( EXIT_FAILURE);
    }      

    flags = 0;
    while ((opt = getopt(argc, argv, opc )) != -1) {
        switch (opt) {
            case 'f':
                flags |= OPT_F;
                strcpy( meta.filename, optarg);
                break;
            case 'k':
                flags |= OPT_K;
                strcpy( meta.entry.key, optarg);
                break;
            case 'v':
                flags |= OPT_V;
                strcpy( val, optarg);
                break;
            case 't':
                flags |= OPT_T;
                strcpy( type, optarg);
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

    if( argc < 3 && argc > 4){
        TRACE_ERR("Incorrect arguments number");
        display_help();
        return(-1);
    }

    if((flags & (OPT_F|OPT_V|OPT_K)) != (OPT_F|OPT_K|OPT_V)){
        TRACE_ERR("Incorrect arguments");
        display_help();
        return(-1);
    }


    if( (flags & OPT_T) != 0){
        if( strcmp( type, "int") == 0){
            meta.entry.value.data_type = DICT_INT;
            meta.entry.value.value.i = strtol( val, NULL, 10);
        }else if( strcmp( type, "uint") == 0){
            meta.entry.value.data_type = DICT_UINT;
            meta.entry.value.value.ui = strtoul( val, NULL, 10);
        }else if( strcmp( type, "float") == 0){
            meta.entry.value.data_type = DICT_FLOAT;
            meta.entry.value.value.f = strtof( val, NULL);
        }else if( strcmp( type, "boolean") == 0){
            meta.entry.value.data_type = DICT_BOOLEAN;
            meta.entry.value.value.b = (val[0] != 0);
        }else if( strcmp( type, "string") == 0){
            meta.entry.value.data_type = DICT_STRING;
            strcpy( meta.entry.value.value.s, val);
            meta.entry.value.data_len = strnlen( val, DICT_MAX_STRLEN);
        }else{
            TRACE_ERR("Invalid data type '%s'", type);
            display_help();
            return( -1);
        }
    }
/*
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

    */
    return( rc);
}


