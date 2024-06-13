#ifndef _KFS_IO_H_
#define _KFS_IO_H_

#define KFS_FILENAME_LEN                           240
typedef struct{
    char kfs_file[KFS_FILENAME_LEN];
    int cache_page_len; 
    int cache_ino_len; 
    int cache_path_len;
    int cache_graph_len; 
    int threads_pool; 
    int max_clients;
}kfs_config_t; 



char *trim (char *s);
uint64_t get_bd_size( char *fname);
int create_file( char *fname);


char *pages_alloc( int n);

int block_read( int fd, char *page, uint64_t addr);
int block_write( int fd, char *page, uint64_t addr);
int extent_read( int fd, char *extent, uint64_t addr, int block_num);
int extent_write( int fd, char *extent, uint64_t addr, int block_num);

int kfs_config_read( char *filename, kfs_config_t *conf);
void kfs_config_display( kfs_config_t *conf);

int kfs_verify( char *filename, int verbose, int extra_verification);

int kfs_open( kfs_config_t *conf);
#endif
