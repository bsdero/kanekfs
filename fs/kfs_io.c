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
#include "kfs_io.h"
#include "kfs_page_cache.h"


/* Cut spaces */
char *trim (char *s){
  /* Initialize start, end pointers */
  char *s1 = s;
  char *s2;

  s2 = s + strlen(s) - 1;

  /* Trim and delimit right side */
  while ( (isspace( (int) *s2)) && (s2 >= s1) ){
      s2--;
  }
  *(s2+1) = '\0';

  /* Trim left side */
  while ( (isspace( (int) *s1)) && (s1 < s2) ){
      s1++;
  }

  /* Copy finished string */
  strcpy (s, s1);
  return( s);
}




uint64_t get_bd_size( char *fname){
#ifdef BLKGETSIZE64
    uint64_t numbytes; 
    int fd = open( fname, O_RDONLY);
    if( fd < 0){
        TRACE_ERR("Could not open block device \n");
        return( -1);
    }
    ioctl( fd, BLKGETSIZE64, &numbytes);
    close(fd);
    return( numbytes);
#else
    TRACE_ERR("Operation not supported\n");
    return( -1);
#endif
}



int create_file( char *fname){
    int fd = creat( fname, 0644);
    if( fd < 0){
        perror( "ERROR: Could not create file \n");
        return( -1);
    }
    close(fd);
    return(0);
}


char *pages_alloc( int n){
    char *page;

    page = malloc( KFS_BLOCKSIZE * n);
    if( page == NULL){
        TRACE_SYSERR( "malloc error.\n");
        return( NULL);
    }

    /* Fill the whole file with zeroes */
    memset( page, 0, (KFS_BLOCKSIZE * n) );
    return( page);
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



int kfs_node_new( int node_num, sinode_t *sinode);
int kfs_node_read( int node_num, sinode_t *sinode);
int kfs_node_remove( int node_num);
int kfs_node_update( int node_num, sinode_t *sinode);
int kfs_node_list_edges( int node_num);
int kfs_node_read_data( int node_num, char *p, size_t len, int offset);
int kfs_node_write_data( int node_num, char *p, size_t len, int offset);

int kfs_mklink( sinode_t *sinode, int node_to, char *name, edge_t *edge);
int kfs_get_edge( sinode_t *sinode, int node_to, kfs_edge_t *edge);
int kfs_update_edge( kfs_edge_t *edge);
int kfs_remove_edge( kfs_edge_t *edge);



