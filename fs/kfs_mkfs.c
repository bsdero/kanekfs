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


#define _1M                                        1048576
#define _1G                                        1073741824

#ifndef min
#define min(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b;       \
})
#endif

int blocks_per_block = (KFS_BLOCKSIZE * KFS_BLOCKSIZE * 8);


int display_help(){
    char *help[]={
        "Usage: kfs_mkfs <file_name> [fs_size]",
        "",
        "    Creates a mxfs filesystem in the specified file name. Works  ",
        "    with the next conditions:                                    ",
        "                                                                 ",
        "      -If <file_name> does not exist, will use the [fs_size]     ",
        "       argument to create the file.                              ", 
        "",
        "      -If <file_name> already exists and is a regular file, will ",
        "       create a filesystem within the file capacity, except the  ",
        "       [fs_size] argument is specified. In this case the next    ",
        "       rules apply:                                              ",
        "         -If [fs_size] argument is bigger than the file size,    ",
        "          zeros will be appended to the file.                    ",
        "         -If [fs_size] argument is lesser or equal than the file ",
        "          size, the filesystem will be created within the file   ",
        "          capacity.                                              ",
        "",
        "      -If <file_name> is a block device, will create a filesystem",
        "       within the file capacity, except the [fs_size] argument   ",
        "       is specified. If this is the case, the fs_size should be  ",
        "       lesser than or equal the block device capacity, and the   ",
        "       filesystem will be created within the block device        ",
        "       capacity.                                                 ",
        "",
        "",
        "-------------------",
        NULL
    };

    char **s = help;
    while( *s){
        printf( "%s\n", *s++);
    }

    return(0);
}




uint64_t validate_size( char *str_size){
    int i;
    int l = strlen( str_size);
    int is_valid = 1;
    uint64_t uint_size; 
    char *ptr;
    unsigned char last;
    
    for( i = 0; i < (l-1); i++){
        if( ! isdigit( str_size[i])){
            is_valid = 0;
            return(0);
        }    
    }

    last = str_size[i];
    if( isdigit( last)){
        uint_size = strtoul( str_size, &ptr, 10);
        return( uint_size);
    }

    if( isalpha( last)){
        uint_size = strtoul( str_size, &ptr, 10);
        last = toupper( last);
        switch( last){
            case 'M': uint_size *= (_1M); 
                      break;
            case 'G': uint_size *= (_1G);
                      break;
            default : return( 0);
        }

        return(  uint_size);
    }

    return(0);
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

uint64_t get_bd_device_size( char *fname){
#ifdef BLKGETSIZE64
    uint64_t numbytes; 
    int fd = open( fname, O_RDONLY);
    if( fd < 0){
        perror("ERROR: Could not open block device \n");
        exit( -1);
    }
    ioctl( fd, BLKGETSIZE64, &numbytes);
    close(fd);
    return( numbytes);
#else
    TRACE("ERROR: Operation not supported\n");
    exit( -1);
    return( -1);
#endif
}


int kfs_create( char *fname, uint64_t fs_size){
    struct stat st;
    int rc;
    int fd; 

#define KFS_CREATE_FILE                 0x0001
#define KFS_SIZE_SPECIFIED              0x0002
#define KFS_IS_BLOCKDEVICE              0x0004
#define KFS_IS_REGULAR_FILE             0x0008
    unsigned int flags = 0x0000; 
    uint64_t num_blocks;
    char *p = NULL;

    rc = stat( fname, &st);
    if( rc != 0){
        printf("File '%s' does not exist, will create. \n", fname);
        flags |= KFS_CREATE_FILE;
        flags |= KFS_IS_REGULAR_FILE;

        /* round to the adequate size */
        num_blocks = fs_size / KFS_BLOCK_SIZE;
        fs_size = num_blocks * KFS_BLOCK_SIZE;
    }else{
        if( (! S_ISREG(st.st_mode)) && (! S_ISBLK(st.st_mode))){
            fprintf( stderr, "ERROR: File is not a regular file nor a ");
            fprintf( stderr, "block device. Exit. ");
            return( -1);
        }  

        if( S_ISREG(st.st_mode)){
            if( st.st_size > 0){
                fs_size = (uint64_t) st.st_size;
            }
            flags |= KFS_IS_REGULAR_FILE;
        }

        if( S_ISBLK(st.st_mode)){
            fs_size = (uint64_t) get_bd_device_size( fname);
            flags |= KFS_IS_BLOCKDEVICE;
        }
    }

    printf(" -File system size: %lu Bytes (%lu MBytes, %.2f GBytes)\n",
        fs_size, fs_size/_1M, (double)fs_size/(double)_1G);

    if( flags & KFS_CREATE_FILE){
        rc = create_file(  fname);
        if( rc != 0){
            exit( -1);
        }
    }

    printf( " -Writing %lu Blocks of %d bytes\n", 
            num_blocks, KFS_BLOCK_SIZE);
    
    fd = open( fname, O_RDWR );
    if( fd < 0){
        perror("ERROR: open error.\n");
        return( -1);
    }

    p = malloc( KFS_BLOCK_SIZE);
    if( p == NULL){
        perror( "ERROR: malloc error.\n");
        close( fd);
        return( -2);
    }

    /* Fill the whole file with zeroes */
    memset( p, 0, KFS_BLOCK_SIZE);
    lseek( fd, 0, SEEK_SET);
    for( int i = 0; i < num_blocks; i++){
        write( fd, p, KFS_BLOCK_SIZE);
        lseek( fd, 0, SEEK_END);
    }
    close( fd); 
    free(p);

    return(0);
}


int build_filesystem_in_file( char *fname, uint64_t fs_size ){

#define KFS_ROOT_INODE_OFFSET          256

    int fd = open( fname, O_RDWR );
    uint64_t i, num_blocks_map;
    char *p, *blockmap;
    mxfs_superblock_t sb; 
    mx_inode_t root_i;    
    time_t current_time;
    char secret[] = "Good! U found the secret message!! 1234567890";
    uint64_t num_blocks = (uint64_t) (fs_size/KFS_BLOCKSIZE);
    uint64_t num_blocks_4_map = ( fs_size / blocks_per_block) + 1;
    uint64_t blockmap_address = num_blocks - num_blocks_4_map;

    printf(" -Block size: %d, Blocks Num: %u\n",
        KFS_BLOCKSIZE, num_blocks);

    printf(" -Blockmap size in blocks: %u, Blockmap Adress: ( 0x%x, %u)\n",
        num_blocks_4_map, blockmap_address, blockmap_address);

    if( fd < 0){
        perror( "ERROR: Could not open file\n");
        return( -1);
    }

    p = malloc( KFS_BLOCKSIZE);
    if( p == NULL){
        perror( "ERROR: malloc error.\n");
        close( fd);
        return( -1);
    }

    /* Fill the whole file with zeroes */
    memset( p, 0, KFS_BLOCKSIZE);
    lseek( fd, 0, SEEK_SET);
    for( i = 0; i < num_blocks; i++){
        write( fd, p, KFS_BLOCKSIZE);
        lseek( fd, 0, SEEK_END);
    }

    /* Build the super block */ 
    memset( (void *) &sb, 0, sizeof( sb));
    sb.magic_number = MXFS_MAGICNUMBER;
    sb.flags = 0x0000;
    sb.block_size = MXFS_BLOCKSIZE;
    sb.num_inodes = 0;
    sb.num_blocks = num_blocks; 
    sb.used_inodes = 1;
    sb.used_blocks = 2 + num_blocks_4_map; /* blockmap + superblock + 
                                            * root inode data */
    sb.block_map = blockmap_address;
    sb.block_map_num = (uint64_t) num_blocks_4_map;
    current_time = time( NULL);
    sb.ctime = (uint64_t) current_time;
    sb.active_root_inode_block = 0;
    sb.active_root_inode_offset = MXFS_ROOT_INODE_OFFSET;
    memset( &sb.spare_bytes, 'x', sizeof( sb.spare_bytes));
    strcpy( sb.spare_bytes, secret);

    /* build the root inode */
    memset( (void *) &root_i, 0, sizeof( root_i));
    root_i.a_time = root_i.c_time = root_i.m_time = current_time; 
    root_i.size = 0;
    root_i.flags = MX_STORAGE_IS_EXTENT; 
    root_i.num_blocks = 1; 
    root_i.mode = S_IFDIR | 666;
    root_i.num_links = 0;
    root_i.uid = 0;
    root_i.gid = 0;

    /* just 1 block for the root directory info, for start */ 
    root_i.storage.extents.extent[0].b_start = 1;
    root_i.storage.extents.extent[0].b_num = 1;


    /* copy the superblock and root inode data to the correct 
     * place in the memory block */
    memset( p, 0, MXFS_BLOCKSIZE);
    memcpy( p, &sb, sizeof( sb));
    memcpy( p + MXFS_ROOT_INODE_OFFSET, &root_i, sizeof( root_i));

    /* write the whole block to disk */
    lseek( fd, 0, SEEK_SET);
    write( fd, p, MXFS_BLOCKSIZE);

    printf(" -Wrote superblock and root inode\n");
    free( p);


    /* almost there, except we need to prepare the block map also. We need to
     * mark block 0 as busy, as the superblock and root inode lives there.  
     * Also the block 1 needs to be marked, as the root inode has an extent 
     * pointing there. 
     * The blockmap used blocks should be marked also. */ 
    blockmap = malloc( MXFS_BLOCKSIZE * num_blocks_4_map);
    if( blockmap == NULL){
        perror( "ERROR: malloc error.\n");
        close( fd);
        return( -1);
    }

    memset( blockmap, 0, MXFS_BLOCKSIZE * num_blocks_4_map);

    /* superblock and root inode */
    bm_set_bit( blockmap, num_blocks, (uint64_t) 0, 1);

    /* superblock's data block */
    bm_set_bit( blockmap, num_blocks, (uint64_t) 1, 1);

    /* used blocks map */
    bm_set_extent( blockmap, 
                   num_blocks, 
                   blockmap_address, 
                   num_blocks_4_map, 
                   1);

     
    lseek( fd, blockmap_address * MXFS_BLOCKSIZE, SEEK_SET);
    write( fd, blockmap, MXFS_BLOCKSIZE * num_blocks_4_map);
    
    close( fd);
    free( blockmap);
    printf(" -Wrote blockmap.\n");
    return(0);
}

int main( int argc, char **argv){
    char *fname;
    char *ssize;
    struct stat st;
    int rc;
    uint64_t fs_size; 
    uint64_t num_blocks;
    uint64_t num_blocks_4_map; 

#define MKFS_CREATE_FILE                 0x0001
#define MKFS_SIZE_SPECIFIED              0x0002
#define MKFS_IS_BLOCKDEVICE              0x0004
#define MKFS_IS_REGULAR_FILE             0x0008

    unsigned int flags = 0x0000; 

    if( argc < 2 || argc > 3){
        fprintf( stderr, "ERROR: Number of arguments not valid\n");
        display_help();
        return( -1);
    }

   
    if( argc == 3){
        fs_size = validate_size( argv[2]);
        if( fs_size == 0){
            fprintf( stderr, "ERROR: Size not valid\n");
            display_help();
            return( -1);
        }
        if( fs_size < 20971520){
            fprintf( stderr, "ERROR: Invalid size, minimum is 20MBytes\n");
            return( -1);
        }
        flags |= MKFS_SIZE_SPECIFIED;
    }


    fname = argv[1];
    rc = stat( fname, &st);
    if( rc != 0){
        printf("File '%s' does not exist, will create. \n", fname);
        flags |= MKFS_CREATE_FILE;
        flags |= MKFS_IS_REGULAR_FILE;         
    }else{
        if( (! S_ISREG(st.st_mode)) && (! S_ISBLK(st.st_mode))){
            fprintf( stderr, "ERROR: File is not a regular file nor a ");
            fprintf( stderr, "block device. Exit. ");
            return( -1);
        }  

        if( S_ISREG(st.st_mode)){
            if( st.st_size > 0){
                fs_size = (uint64_t) st.st_size;
            }
            flags |= MKFS_IS_REGULAR_FILE;
        }

        if( S_ISBLK(st.st_mode)){
            fs_size = (uint64_t) get_bd_device_size( fname);
            flags |= MKFS_IS_BLOCKDEVICE;
        }
    }

    printf(" -File system size: %u Bytes (%u MBytes, %.2f GBytes)\n",
        fs_size, fs_size/_1M, (double)fs_size/(double)_1G);

    if( flags & MKFS_CREATE_FILE){
        rc = create_file(  fname);
        if( rc != 0){
            exit( -1);
        }
    }
  
    rc = build_filesystem_in_file( fname, fs_size); 
    if( rc != 0){
        fprintf( stderr, "ERROR: Failed to build a filesystem\n");
    }else{
        printf("\nDone.\n");
    }

    return( rc);
}

