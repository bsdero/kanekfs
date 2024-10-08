#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "kfs.h"

#define KFS_SERVER_CONF                            "/etc/kfs/kfs.conf"
#define OPT_K                            0x02
#define OPT_V                            0x04
#define OPT_T                            0x08
#define OPT_HELP                         0x10
#define OPT_CONF                         0x20


typedef struct{
    char filename[250];
    dict_entry_t entry;
}sbmeta_t;


typedef struct{
    char conf_file[KFS_FILENAME_LEN];
    unsigned int flags;
    sbmeta_t meta;
}options_t;


int display_help(){
    char *help[]={
        "Usage: kfs_sbsetmeta <-c conf> <-k key> <-v value> [-t datatype]  ",
        "",
        "    kfs_sbsetmeta adds one key-value pair to the slot #0 of the   ",
        "    file system. The slot #0 is exclusive for the superblock.     ",
        "",
        "       <-c conf> Configuration file which includes a file or a    ",
        "       block device configuration with a valid KanekFS ",
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


int parse_opts( int argc, char **argv, options_t *options){

    char opc[] = "c:k:v:t:ih";
    sbmeta_t meta;
    char val[DICT_MAX_STRLEN];
    char type[128];
    int opt, flags = 0;

    memset( (void *) options, 0, sizeof( options_t));
    memset( (void *) &meta, 0, sizeof( sbmeta_t));

    if( argc == 0){
        fprintf(stderr, "ERROR: invalid number of arguments.\n");
        display_help();
        exit( EXIT_FAILURE);
    }      

    flags = 0;
    while ((opt = getopt(argc, argv, opc )) != -1) {
        switch (opt) {
            case 'c':
                flags |= OPT_CONF;
                strcpy( options->conf_file, optarg);
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

    if( argc < 6 && argc > 8){
        TRACE_ERR("Incorrect arguments number");
        display_help();
        return(-1);
    }

    if((flags & (OPT_CONF|OPT_V|OPT_K)) != (OPT_CONF|OPT_K|OPT_V)){
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

    if( (flags & OPT_CONF ) == 0){
        snprintf( options->conf_file, KFS_FILENAME_LEN, "%s", KFS_SERVER_CONF);
        flags |= OPT_CONF;
    }

    options->flags = flags;
    options->meta = meta;

    return(0);
}

int main( int argc, char **argv){
    int rc;
    options_t options;
    kfs_config_t config;
    dict_entry_t e;
    value_t v;
    slot_t slot;

    rc = parse_opts( argc, argv, &options);
    if( rc < 0){
        return( rc);
    }

    rc = kfs_config_read( options.conf_file, &config);
    if( rc < 0){
        return( rc);
    }


    rc = kfs_mount( &config);
    if( rc < 0){
        TRACE_ERR("kfs_mount() failed");
        return( rc);
    }


    e = options.meta.entry;

    v = e.value;
    printf("Meta:\n");
    printf(" %s:%s=", e.key, dict_get_type_name( v.data_type));
    switch( v.data_type){
        case DICT_INT: 
            printf("%d ", v.value.i); 
            break;
        case DICT_UINT: 
            printf("%lu ", v.value.ui); 
            break;
        case DICT_FLOAT: 
            printf("%6.4f ", v.value.f); 
            break;
        case DICT_BOOLEAN: 
            printf("%c ", (0x30 + v.value.b)); 
            break;
        case DICT_STRING: 
            printf("'%s' ", v.value.s); 
            break;
        case DICT_BLOB: 
        case DICT_EXTENT:
            printf("BLOB/EXTENT "); 
            break;
        default: 
            printf(" WTF "); 
    }
    printf("\n");



    rc = kfs_umount();
    if( rc < 0){
        return( rc);
    }


    return(0);
}

