#ifndef _EIO_H_
#define _EIO_H_

uint64_t get_bd_size( char *fname);
int create_file( char *fname);
char *pages_alloc( int n);
int block_read( int fd, char *page, uint64_t addr);
int block_write( int fd, char *page, uint64_t addr);
int extent_read( int fd, char *extent, uint64_t addr, int block_num);
int extent_write( int fd, char *extent, uint64_t addr, int block_num);



#endif

