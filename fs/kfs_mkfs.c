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


#define OPT_SIZE                                   0x0001
#define OPT_NUM_SLOTS                              0x0002
#define OPT_NUM_SINODES                            0x0004
#define OPT_CALCULATE                              0x0008
#define OPT_PERCENTAGE                             0x0020
#define OPT_VERBOSE                                0x0040
#define OPT_HELP                                   0x0080

#define MKFS_CREATE_FILE                           0x0100
#define MKFS_IS_BLOCKDEVICE                        0x0200
#define MKFS_IS_REGULAR_FILE                       0x0400

#define MKFS_DEFAULT_PERCENTAGE                    10




uint64_t blocks_per_block = KFS_BLOCKSIZE * 8;
typedef struct{
    char filename[240];
    char str_size[32];
    char str_slots_num[24];
    char str_sinodes_num[24];
    char str_percentage[10];
    uint64_t sinodes_num;
    uint64_t slots_num;
    uint64_t size;
    int percentage;
    int flags;
}options_t;

typedef struct{
    uint64_t in_sinodes_num;
    uint64_t in_slots_num;
    uint64_t in_file_size_in_mbytes;
    uint64_t in_file_size_in_blocks;
    int in_percentage;
    uint64_t out_bitmap_size_in_blocks;
    uint64_t out_slots_table_in_blocks, out_slots_bitmap_blocks_num;
    uint64_t out_sinodes_table_in_blocks, out_sinodes_bitmap_blocks_num;
    uint64_t out_sinodes_num, out_slots_num;
    uint64_t out_total_blocks_required;
    int result;
}blocks_calc_t;


#define PG_SB                                      0
#define PG_MAP                                     1

char *pages[]={ NULL, NULL};

options_t options;
blocks_calc_t blocks_calc;

#define PRINTV(fmt,...)          {                                          \
                                     if( options.flags & OPT_VERBOSE){      \
                                        fprintf( stdout,                    \
                                                 fmt"\n", ##__VA_ARGS__);   \
                                     }                                      \
                                 } \


int display_help(){
    char *help[]={
        "Usage: kfs_mkfs [options] file_name",
        "",
        "    Options:",
        "      -s fs_size        File size",
        "      -i sino_num       Number of Super Inodes",
        "      -n slot_num       Number of Metadata Slots", 
        "      -c                Calc number of items",
        "      -p percentage     super inodes and slots percentage", 
        "      -v                Verbose mode",
        "      -h                Help",
        "",
        "    kfs_mkfs creates a kfs filesystem in the specified file name.",
        "    It works with the next conditions:                           ",
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
        "      -For the [-s fs_size], the expected value is the size in   ",
        "       MBytes. Also the characters 'M' and 'G' after the number  ",
        "       specifies if we refer to MBytes or GBytes. Examples:      ",
        "       20    20 MByes,                                           ",
        "       512M  512 MBytes,                                         ",
        "       8G    8 GBytes                                            ", 
        "",
        "      -If <file_name> is a block device, will create a filesystem",
        "       within the file capacity, except the [fs_size] argument   ",
        "       is specified. If this is the case, the [fs_size] should be",
        "       lesser than or equal the block device capacity, and the   ",
        "       filesystem will be created within the block device        ",
        "       capacity.                                                 ",
        "",
        "      -If [-c] option is specified, nothing is touched, but the  ",
        "       number of slots, super inodes, blocks, etc required, is   ",
        "       shown. If the [fs_size] is passed, and/or the size        ",
        "       detected, the number of slots and superinodes and used    ",
        "       blocks are shown. If the [sino_num] or [slot_num] of both ",
        "       are passed, the calculations showns the number of blocks  ", 
        "       and device size which is required for that calculations.  ",
        "",
        "      -If [-p] is specified, that percentage is used for the     ",
        "       super inodes and slots. The default is about 12%. It needs",
        "       to know the disk size before, so it can be used with -s   ",
        "       and -c.",
        "",
        "      -Options [-s, -i, -n, -c] can be used together.",
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



uint64_t validate_size( char *str_size){
    int i;
    int l = strlen( str_size);
    uint64_t uint_size; 
    char *ptr;
    unsigned char last;
    
    for( i = 0; i < (l-1); i++){
        if( ! isdigit( (int) str_size[i])){
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


uint64_t validate_num( char *str_size){
    int i;
    int l = strlen( str_size);
    uint64_t uint; 
    char *ptr;
    
    for( i = 0; i < l; i++){
        if( ! isdigit( (int) str_size[i])){
            return(0);
        }    
    }

    uint = strtoul( str_size, &ptr, 10);
    return( uint);
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


int page_write( int fd, char *page, int block_num){
    lseek( fd, block_num * KFS_BLOCKSIZE, SEEK_SET);
    write( fd, page, KFS_BLOCKSIZE);

    return(0);
}


int build_superblock( int fd){
    time_t current_time;
    kfs_extent_t extent;
    kfs_superblock_t *sb = (kfs_superblock_t *) pages[PG_SB]; 
    int rc; 
    char *p;
    blocks_calc_t *bc = &blocks_calc;
    char secret[] = "Good! U found the secret message!! 1234567890";
    uint64_t sinode_map_block, slot_map_block, fs_map_block;


    PRINTV("-Building superblock");

    /* set maps addresses */
    fs_map_block = bc->in_file_size_in_blocks - 
                   bc->out_bitmap_size_in_blocks;

    slot_map_block = fs_map_block - bc->out_slots_bitmap_blocks_num;
    sinode_map_block = slot_map_block - bc->out_sinodes_bitmap_blocks_num;

    /* Build the super block */ 
    memset( (void *) sb, 0, sizeof( kfs_superblock_t ));
    current_time = time( NULL);
    sb->sb_magic = KFS_MAGIC;
    sb->sb_version = 0x000001;
    sb->sb_flags = 0x0000;
    sb->sb_blocksize = KFS_BLOCKSIZE;
    sb->sb_root_super_inode = 0;
   

    sb->sb_c_time = current_time; 
    sb->sb_m_time = current_time; 
    sb->sb_a_time = current_time;

    /* populate the sinode index extent and its map extent */
    sb->sb_si_table.capacity = bc->out_sinodes_num;
    sb->sb_si_table.in_use = 1;
    extent.ee_block_addr = 1; 
    extent.ee_block_size = bc->out_sinodes_table_in_blocks;
    extent.ee_log_size = bc->out_sinodes_num;
    extent.ee_log_addr = 0;
    sb->sb_si_table.table_extent = extent;

    extent.ee_block_addr = sinode_map_block;
    extent.ee_block_size = bc->out_sinodes_bitmap_blocks_num;
    extent.ee_log_size = bc->out_sinodes_bitmap_blocks_num;
    extent.ee_log_addr = 0;
    sb->sb_si_table.bitmap_extent = extent;



    /* populate the slot index extent and its map extent */
    sb->sb_slot_table.capacity = bc->out_slots_num;
    sb->sb_slot_table.in_use = 0;
    extent.ee_block_addr = bc->out_sinodes_table_in_blocks + 1; 
    extent.ee_block_size = bc->out_slots_table_in_blocks;
    extent.ee_log_size = bc->out_slots_num;
    extent.ee_log_addr = 0;
    sb->sb_slot_table.table_extent = extent;

    extent.ee_block_addr = slot_map_block;
    extent.ee_block_size = bc->out_slots_bitmap_blocks_num;
    extent.ee_log_size = bc->out_slots_bitmap_blocks_num;
    extent.ee_log_addr = 0;
    sb->sb_slot_table.bitmap_extent = extent;

    /* populate the map extent */
    extent.ee_block_addr = fs_map_block;
    extent.ee_block_size = bc->out_bitmap_size_in_blocks;
    extent.ee_log_size = bc->in_file_size_in_blocks;
    extent.ee_log_addr = 0;
    sb->sb_blockmap.bitmap_extent = extent;
    memset( ( void *) &extent, 0, sizeof( kfs_extent_t));
    sb->sb_blockmap.table_extent = extent;
 
    p = ( char *) sb;
    p += 512; 
    strcpy( p, secret);
    PRINTV("    -Write Superblock in block: [0]")
    rc = page_write( fd, (void *) sb, 0);
    if( rc != 0){
        TRACE_ERR( "Could not write\n");
        exit(-1);
    }

    return( rc);
}


int build_map( int fd){
    kfs_extent_header_t *ex_header = NULL;
    char *p, *bitmap;
    blocks_calc_t *bc = &blocks_calc;
    int rc = 0; 

    PRINTV("-Building Filesystem Block Map");

    p = pages_alloc( bc->out_bitmap_size_in_blocks);
    pages[PG_MAP] = p;
    ex_header = (kfs_extent_header_t *) p;

    ex_header->eh_magic = KFS_BLOCKMAP_MAGIC;
    ex_header->eh_entries_in_use = 0;
    ex_header->eh_entries_capacity = bc->in_file_size_in_blocks;
    ex_header->eh_flags = KFS_ENTRIES_ROOT|KFS_ENTRIES_LEAF;
    bitmap = p + sizeof( kfs_extent_header_t);


    /* set superblock */
    bm_set_bit( ( unsigned char *) bitmap, bc->in_file_size_in_blocks, 0, 1);
    ex_header->eh_entries_in_use = 1;



    return(rc);
}

int build_sinodes( int fd){
    time_t current_time;
    kfs_extent_header_t *ex_header = NULL;
    kfs_sinode_t *sino;
    int block_num; 
    uint64_t i, sino_num;
    char *p, *slp;
    unsigned char *bitmap;
    blocks_calc_t *bc = &blocks_calc;
    uint64_t sinode_map_block, slot_map_block, fs_map_block, liminf, limsup;


    PRINTV("-Building Super inodes table");

    /* set maps addresses */
    fs_map_block = bc->in_file_size_in_blocks - 
                   bc->out_bitmap_size_in_blocks;

    slot_map_block = fs_map_block - bc->out_slots_bitmap_blocks_num;
    sinode_map_block = slot_map_block - bc->out_sinodes_bitmap_blocks_num;


    slp = p = pages_alloc( 1);
    ex_header = (kfs_extent_header_t *) p;

    current_time = time( NULL);

    /* fill in the first block of the sinodes table */
    ex_header->eh_magic = KFS_SINODE_TABLE_MAGIC;
    ex_header->eh_entries_in_use = 1;
    ex_header->eh_entries_capacity = bc->out_sinodes_num;
    ex_header->eh_flags = KFS_ENTRIES_ROOT|KFS_ENTRIES_LEAF;
    i = sizeof( kfs_extent_header_t);

    /* fill in the first inode */
    sino_num = 0;
    sino = ( kfs_sinode_t *) slp + sizeof( kfs_extent_header_t);
    sino->si_a_time = current_time;
    sino->si_c_time = current_time;
    sino->si_m_time = current_time;
    sino->si_slot_id = 0xfffffffe;
    sino->si_id = sino_num++;

    /* fill in the other inodes */
    i = sizeof( kfs_extent_header_t) + sizeof( kfs_sinode_t);
    while( i < KFS_BLOCKSIZE && sino_num < bc->out_sinodes_num){
        sino->si_id = sino_num++;
        sino++;
        i += sizeof( kfs_sinode_t);
    }

    /* write the first block of the extent with the sinodes table */
    liminf = block_num = 1;
    page_write( fd, slp, block_num++);

    /* write out the remaining blocks of the table. 
     * -1 because we already wrote 1 block 
     */
    while( block_num <= bc->out_sinodes_table_in_blocks - 1){
        memset( (void *) slp, 0, KFS_BLOCKSIZE);
 
        i = 0;
        sino = ( kfs_sinode_t *) p;

        while( i < KFS_BLOCKSIZE && sino_num < bc->out_sinodes_num){
            sino->si_id = sino_num++;
            sino++;
            i += sizeof( kfs_sinode_t);
        }

        page_write( fd, p, block_num++);
    }

    limsup = block_num;
    PRINTV("    -Writing SuperInodes table blocks: [%lu-%lu]",
                liminf,
                limsup); 
 
    free( p);


    /* now on to the super inodes map */
    PRINTV("    -Building super inodes map");

    slp = p = pages_alloc( bc->out_sinodes_bitmap_blocks_num);


    /* fill in the super inodes map */
    memset( (void *) p, 0, KFS_BLOCKSIZE);
    ex_header = (kfs_extent_header_t *) p;

    ex_header->eh_magic = KFS_SINODE_BITMAP_MAGIC;
    ex_header->eh_entries_in_use = 1;
    ex_header->eh_entries_capacity = bc->out_sinodes_num;
    ex_header->eh_flags = KFS_ENTRIES_ROOT|KFS_ENTRIES_LEAF;
    bitmap = (unsigned char *) p + sizeof( kfs_extent_header_t);

    bm_set_bit( ( unsigned char *) bitmap, bc->out_sinodes_num, 0, 1);
    liminf = sinode_map_block;
    for( i = 0; i < bc->out_sinodes_bitmap_blocks_num; i++){
        page_write( fd, slp, sinode_map_block + i);
        slp += KFS_BLOCKSIZE;
    }
    limsup = sinode_map_block + i - 1;
    PRINTV("    -Writing SuperInodes blocks map: [%lu-%lu]", 
                liminf, 
                limsup);
    
    free( p);

    /* update the file system block map */
    p = pages[PG_MAP];
    ex_header = (kfs_extent_header_t *) p;
    bitmap = (unsigned char *) p + sizeof( kfs_extent_header_t);


    /* mark the bits corresponding to the super inodes table */
    bm_set_extent( bitmap, 
                   bc->in_file_size_in_blocks, 
                   1,
                   bc->out_sinodes_table_in_blocks,
                   1);
    ex_header->eh_entries_in_use += bc->out_sinodes_table_in_blocks;
  
    /* mark the bits corresponding to the super inodes map */
    bm_set_extent( bitmap, 
                   bc->in_file_size_in_blocks, 
                   sinode_map_block,
                   bc->out_sinodes_bitmap_blocks_num,
                   1);
    ex_header->eh_entries_in_use += bc->out_sinodes_bitmap_blocks_num;
 
    return(0);
}


int build_slots( int fd){
    kfs_extent_header_t *ex_header = NULL;
    kfs_slot_t *slot;
    uint64_t block_num, i, slot_num;
    char *p, *slp;
    unsigned char *bitmap;
    blocks_calc_t *bc = &blocks_calc;
    uint64_t slot_map_block, fs_map_block, liminf, limsup;


    PRINTV("-Building Slots table");

    /* set maps addresses */
    fs_map_block = bc->in_file_size_in_blocks - 
                   bc->out_bitmap_size_in_blocks;

    slot_map_block = fs_map_block - bc->out_slots_bitmap_blocks_num;


    slp = p = pages_alloc( 1);
    ex_header = (kfs_extent_header_t *) p;

    /* fill in the first block of the slots table */
    ex_header->eh_magic = KFS_SLOTS_TABLE_MAGIC;
    ex_header->eh_entries_in_use = 0;
    ex_header->eh_entries_capacity = bc->out_slots_num;
    ex_header->eh_flags = KFS_ENTRIES_ROOT|KFS_ENTRIES_LEAF;
    i = sizeof( kfs_extent_header_t);

    /* fill in the slots */
    slot_num = 0;
    slot = ( kfs_slot_t *) slp + sizeof( kfs_extent_header_t);

    while( i < KFS_BLOCKSIZE && slot_num < bc->out_slots_num){
        slot->slot_id = slot_num++;
        slot++;
        i += sizeof( kfs_slot_t);
    }

    /* write the first block of the extent with the slots table */
    block_num = bc->out_sinodes_table_in_blocks + 1;
    liminf = block_num;
    page_write( fd, slp, block_num++);
    limsup = liminf + bc->out_slots_table_in_blocks;

    /* write out the remaining blocks of the table */
    while( block_num <= limsup){
        memset( (void *) slp, 0, KFS_BLOCKSIZE);
 
        i = 0;
        slot = ( kfs_slot_t *) p;

        while( i < KFS_BLOCKSIZE && slot_num < bc->out_slots_num){
            slot->slot_id = slot_num++;
            slot++;
            i += sizeof( kfs_slot_t);
        }

        page_write( fd, p, block_num++);
    }
    free( p);

    PRINTV("    -Writing slots table blocks: [%lu-%lu]", 
                liminf, 
                limsup);
 
    /* now on to the super inodes map */
    PRINTV("    -Building slots map");

    slp = p = pages_alloc( bc->out_slots_bitmap_blocks_num);


    /* fill in the super inodes map */
    memset( (void *) p, 0, KFS_BLOCKSIZE);
    ex_header = (kfs_extent_header_t *) p;

    ex_header->eh_magic = KFS_SLOTS_BITMAP_MAGIC;
    ex_header->eh_entries_in_use = 0;
    ex_header->eh_entries_capacity = bc->out_slots_num;
    ex_header->eh_flags = KFS_ENTRIES_ROOT|KFS_ENTRIES_LEAF;
    bitmap = (unsigned char *) p + sizeof( kfs_extent_header_t);
    liminf = slot_map_block;
    for( i = 0; i < bc->out_slots_bitmap_blocks_num; i++){
        page_write( fd, slp, slot_map_block + i);
        slp += KFS_BLOCKSIZE;
    }
    limsup = slot_map_block + i - 1;
    PRINTV("    -Writing Slots blocks map: [%lu-%lu]", 
                liminf, 
                limsup);
 
    free( p);

    /* update the file system block map */
    p = pages[PG_MAP];
    ex_header = (kfs_extent_header_t *) p;
    bitmap = (unsigned char *) p + sizeof( kfs_extent_header_t);


    /* mark the bits corresponding to the super inodes table */
    bm_set_extent( bitmap, 
                   bc->in_file_size_in_blocks, 
                   bc->out_sinodes_table_in_blocks + 1,
                   bc->out_slots_table_in_blocks,
                   1);
    ex_header->eh_entries_in_use += bc->out_slots_table_in_blocks;
  
    /* mark the bits corresponding to the super inodes map */
    bm_set_extent( bitmap, 
                   bc->in_file_size_in_blocks, 
                   slot_map_block,
                   bc->out_slots_bitmap_blocks_num,
                   1);
    ex_header->eh_entries_in_use += bc->out_slots_bitmap_blocks_num;
 
    return(0);
}


int write_map(int fd){
    char *p = pages[PG_MAP];
    uint64_t i, block_num, fs_map_block, liminf, limsup; 
    blocks_calc_t *bc = &blocks_calc;

    PRINTV("-Writing block map");

    /* set maps addresses */
    fs_map_block = bc->in_file_size_in_blocks - bc->out_bitmap_size_in_blocks;
    block_num = bc->out_bitmap_size_in_blocks;
    liminf = fs_map_block;
    for( i = 0; i < block_num; i++){
        page_write( fd, p, fs_map_block++);
        p += KFS_BLOCKSIZE;
    }

    limsup = fs_map_block - 1;

    PRINTV("    -Writing KFS block map: [%lu-%lu]", 
                 liminf, 
                 limsup);
    
    fsync( fd);
    free( pages[PG_MAP]);

    return(0);
}


int build_filesystem_in_file(){
    int fd, rc = 0;
    uint64_t i;
    char *page;
    blocks_calc_t *bc = &blocks_calc;
    
    PRINTV("-Creating file");
    if( options.flags & MKFS_CREATE_FILE){
        rc = create_file( options.filename);
        if( rc != 0){
            return( rc);
        }
    }

    PRINTV("-Writing file");
    fd = open( options.filename, O_RDWR );
    if( fd < 0){
        TRACE_ERR( "ERROR: Could not open file '%s'\n", options.filename);
        return( -1);
    }

    page = pages_alloc( 1);
    if( page == NULL){
        close( fd);
        exit(-1);
    }


    lseek( fd, 0, SEEK_SET);
    for( i = 0; i < bc->in_file_size_in_blocks; i++){
        write( fd, page, KFS_BLOCKSIZE);
        //lseek( fd, 0, SEEK_END);
    }
    fsync( fd);



    pages[PG_SB] = page;
    rc = build_superblock( fd);
    if( rc < 0){
        return( rc);
    }


    rc = build_map( fd);
    if( rc < 0){
        return( rc);
    }


    rc = build_sinodes( fd); 
    if( rc < 0){
        return( rc);
    }


    rc = build_slots( fd); 
    if( rc < 0){
        return( rc);
    }


    rc = write_map( fd);
    if( rc < 0){
        return( rc);
    }

   close( fd);

    return( rc);
}


/* display calculations results */
int blocks_results_display(){
    blocks_calc_t *bc = &blocks_calc;

    printf("IN:\n");
    printf("    Blocksize=%d\n", KFS_BLOCKSIZE);
    printf("    InodesNum=%lu\n", bc->in_sinodes_num);
    printf("    SlotsNum=%lu\n", bc->in_slots_num);
    printf("    FileSizeInMB=%lu\n", bc->in_file_size_in_mbytes);
    printf("    FileSizeInBlocks=%lu\n", bc->in_file_size_in_blocks);
    printf("    Percentage=%d\n", bc->in_percentage);
    printf("OUT:\n");
    printf("    TotalBlocksRequired=%lu\n", bc->out_total_blocks_required);
    printf("    TotalDiskRequiredInMB=%lu\n",  /* 128 blocks per Mbyte */
           bc->out_total_blocks_required / 128);
    printf("    BitmapSizeInBlocks=%lu\n", bc->out_bitmap_size_in_blocks);
    printf("    InodesNum=%lu\n", bc->out_sinodes_num);
    printf("    SlotsNum=%lu\n", bc->out_slots_num);
    printf("    SlotsTableInBlocks=%lu, SlotsMapInBlocks=%lu\n", 
        bc->out_slots_table_in_blocks, bc->out_slots_bitmap_blocks_num);
    printf("    SinodesTableInBlocks=%lu, SinodesMapInBlocks=%lu\n", 
        bc->out_sinodes_table_in_blocks, bc->out_sinodes_bitmap_blocks_num);

    return(0);
}


/* In function of the arguments passed, calculate how many slots and super
 * inodes will be created, or the device size required. Also fill in 
 * how many blocks are required for the corresponding tables and bitmaps. 
 */
int compute_blocks(){
    blocks_calc_t *bc = &blocks_calc;

    uint64_t total_blocks_required = 0, bitmap_size_in_blocks = 0;
    uint64_t slots_per_block;
    uint64_t slots_blocks, slots_map_blocks, slots_result;
    uint64_t sinodes_per_block;
    uint64_t sinodes_blocks, sinodes_map_blocks, sinodes_result;
    uint64_t slots_in_1st_block, sinodes_in_1st_block;
    uint64_t bits_per_block, n;

    /* clean ddata structure */
    memset( ( void *) bc, 0, sizeof( blocks_calc_t));

    /* fill in sizes */
    bc->in_file_size_in_blocks = options.size / KFS_BLOCKSIZE;
    bc->in_file_size_in_mbytes = options.size / _1M;
 
    if( options.flags & (OPT_NUM_SINODES|OPT_NUM_SLOTS)){
        
        bc->in_sinodes_num = options.sinodes_num; 
        bc->in_slots_num = options.slots_num;
    }
    

    bc->in_percentage = options.percentage;
 
    /* do first real computations */
    n =  KFS_BLOCKSIZE - sizeof( kfs_extent_header_t); 
    slots_in_1st_block = n / sizeof( kfs_slot_t);
    slots_per_block = KFS_BLOCKSIZE / sizeof( kfs_slot_t);


    sinodes_in_1st_block = n / sizeof( kfs_sinode_t);
    sinodes_per_block = KFS_BLOCKSIZE / sizeof( kfs_sinode_t);
 

    /* now calculate how many bits per block can have our bitmap */
    bits_per_block = (KFS_BLOCKSIZE - sizeof( kfs_extent_header_t)) * 8;

    /* given the file/device size, and the percentage, calculate how
     * many super inodes and slots do we need */
    if( bc->in_sinodes_num == 0 && bc->in_slots_num == 0){
        total_blocks_required += 1; /* super block */

        /* calculate how many blocks for slots do we have */
        slots_blocks = (uint64_t) ( (float)bc->in_file_size_in_blocks * 
                        (( float) bc->in_percentage / 100.0) / 2.0);
        sinodes_blocks = slots_blocks;  /* same for sinodes table blocks */

        /* sum the total blocks required now */
        total_blocks_required += slots_blocks + sinodes_blocks;

        /* how many blocks our slots map need */
        slots_map_blocks = ( slots_blocks / bits_per_block) + 1;

        /* real number of blocks for slots we have */
        slots_blocks -= slots_map_blocks;

        /* now get the real slots number we can have */
        slots_result = ((slots_blocks - 1) * slots_per_block) + 
                       slots_in_1st_block;


        bc->out_slots_bitmap_blocks_num = slots_map_blocks;
        bc->out_slots_table_in_blocks = slots_blocks;
        bc->out_slots_num = slots_result;

        /* calculate how many super inodes are available, from the file size 
         * in blocks  */
 
        /* how many blocks our super inodes map need */
        sinodes_map_blocks = ( sinodes_blocks / bits_per_block) + 1;

        /* real number of blocks for super inodes we have */
        sinodes_blocks -= sinodes_map_blocks;

        /* now get the real super inodes number we can have */
        sinodes_result = ((sinodes_blocks - 1) * sinodes_per_block) + 
                         sinodes_in_1st_block;


        bc->out_sinodes_bitmap_blocks_num = sinodes_map_blocks;
        bc->out_sinodes_table_in_blocks = sinodes_blocks;
        bc->out_sinodes_num = sinodes_result;
    }else{ /* we have a fixed number of super inodes and slots passed in
              from the CLI, so calculate how many blocks are required and 
              how big the file/device should  be. */
        if( bc->in_sinodes_num != 0){
            sinodes_blocks = ( (bc->in_sinodes_num - sinodes_in_1st_block) / 
                               sinodes_per_block) + 2;
            sinodes_map_blocks = ( sinodes_blocks / bits_per_block) + 1;
            sinodes_blocks -= sinodes_map_blocks;
            bc->out_sinodes_bitmap_blocks_num = sinodes_map_blocks;
            bc->out_sinodes_table_in_blocks = sinodes_blocks;
            total_blocks_required += sinodes_map_blocks + sinodes_blocks;
            bc->out_sinodes_num = bc->in_sinodes_num;
        }

        if( bc->in_slots_num != 0){
            slots_blocks = ( (bc->in_slots_num - slots_in_1st_block) / 
                                slots_per_block) + 2;
            /* how many blocks our slots map need */
            slots_map_blocks = ( slots_blocks / bits_per_block) + 1;
            slots_blocks -= slots_map_blocks;
            bc->out_slots_bitmap_blocks_num = slots_map_blocks;
            bc->out_slots_table_in_blocks = slots_blocks;
            total_blocks_required += slots_map_blocks + slots_blocks;
            bc->out_slots_num = bc->in_slots_num;
        }

        /* calculate total device size */
        n = total_blocks_required;
        bc->in_file_size_in_blocks = (uint64_t) ((float) n * 
                                        ( 100.0 / (float) bc->in_percentage));

        /* minus slots and sinodes part 
        bc->in_file_size_in_blocks -= total_blocks_required; 
        */
        bc->in_file_size_in_mbytes = bc->in_file_size_in_blocks / 128;
        total_blocks_required += 1;
    }


    /* now fill in the bitmap size in blocks and finish calculations */
    bitmap_size_in_blocks = (bc->in_file_size_in_blocks / bits_per_block) + 1;
    bc->out_bitmap_size_in_blocks = bitmap_size_in_blocks;
    total_blocks_required += bitmap_size_in_blocks;

    bc->out_total_blocks_required = total_blocks_required;

    return(0);
}


/* parse the passed options */
int parse_opts( int argc, char **argv){
    int opt, flags; 
    struct stat st;
    int rc;
    int need_file = 1;

    flags = 0;
    char opc[] = "s:i:n:cp:vh";
    memset( (void *) &options, 0, sizeof( options_t));

    if( argc == 0){
        fprintf(stderr, "ERROR: invalid number of arguments.\n");
        display_help();
        exit( EXIT_FAILURE);
    }      

    
    while ((opt = getopt(argc, argv, opc )) != -1) {
        switch (opt) {
            case 's':
                flags |= OPT_SIZE;
                strcpy( options.str_size, optarg);
                break;
            case 'i':
                flags |= OPT_NUM_SINODES;
                strcpy( options.str_sinodes_num, optarg);
                break;
            case 'n':
                flags |= OPT_NUM_SLOTS;
                strcpy( options.str_slots_num, optarg);
                break;
            case 'p':
                flags |= OPT_PERCENTAGE;
                strcpy( options.str_percentage, optarg);
                break;
            case 'v':
                flags |= OPT_VERBOSE;
                break;
            case 'c':
                flags |= OPT_CALCULATE;
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

    /* determine if the final file name is required. If we are just
     * doing computations, this is not needed. */
    if( ( flags & (OPT_CALCULATE|OPT_NUM_SINODES|OPT_NUM_SLOTS)) ==
                  (OPT_CALCULATE|OPT_NUM_SINODES|OPT_NUM_SLOTS)){
        need_file = 0;
    }


    if( need_file == 1){
        /* last argument missing, error .*/
        if (optind >= argc) {
            fprintf(stderr, "Expected argument after options\n");
            display_help();
            exit(EXIT_FAILURE);
        }
    }

    /* cases for invalid options combination */
    if(  ( flags & OPT_HELP) || /* any option with -h */
         ( flags == OPT_VERBOSE) ||   /* -v alone */

         /* -p and -i, or -p and -n can not go together */
         ( (flags & ( OPT_PERCENTAGE | OPT_NUM_SINODES)) == 
                    ( OPT_PERCENTAGE | OPT_NUM_SINODES)) ||
         ( (flags & ( OPT_PERCENTAGE | OPT_NUM_SLOTS)) ==
                    ( OPT_PERCENTAGE | OPT_NUM_SLOTS))

        ){
        fprintf( stderr, "Invalid options combination.\n");
        display_help();
        exit( EXIT_FAILURE);
    }


    if( need_file != 0){
        strcpy( options.filename, argv[optind]);
    }

    options.flags = flags;

    /* validate the specified options */
    if( options.flags & OPT_SIZE){
        options.size = validate_size( options.str_size);
        if( options.size == 0){
            fprintf( stderr, "ERROR: Size not valid\n");
            display_help();
            return( -1);
        }
#define MIN_FS                           10485760 /* 10 Mb */
/*define MIN_FS                          20971520 */
        
        if( options.size < MIN_FS){
            fprintf( stderr, 
                     "ERROR: Invalid size, minimum is %d MBytes\n",
                     MIN_FS / _1M);
            return( -1);
        }
    }

   if( options.flags & OPT_PERCENTAGE){
        options.percentage = validate_num( options.str_percentage);
        if( options.percentage < 2 || options.percentage > 90 ){
            fprintf( stderr, "ERROR: Percentage not valid. Valid range is ");
            fprintf( stderr, "[2-90]\n");
            display_help();
            return( -1);
        }
    }else{
        options.percentage = MKFS_DEFAULT_PERCENTAGE;
    }



    if( options.flags & OPT_NUM_SINODES){
        options.sinodes_num = validate_num( options.str_sinodes_num);
        if( options.sinodes_num == 0){
            fprintf( stderr, "ERROR: SInodes num not valid\n");
            display_help();
            return( -1);
        }
    }
    if( options.flags & OPT_NUM_SLOTS){
        options.slots_num = validate_num( options.str_slots_num);
        if( options.slots_num == 0){
            fprintf( stderr, "ERROR: Slots num not valid\n");
            display_help();
            return( -1);
        }
    }


    if( need_file == 1){
        rc = stat( options.filename, &st);
        if( rc != 0){
            if( (options.flags & OPT_CALCULATE) == 0 ){
                if( options.flags & OPT_VERBOSE){
                    printf( "File '%s' does not exist, will create. \n", 
                            options.filename);
                }
                options.flags |= MKFS_CREATE_FILE;
                options.flags |= MKFS_IS_REGULAR_FILE;
            }
        }else{
            if( (! S_ISREG(st.st_mode)) && (! S_ISBLK(st.st_mode))){
                fprintf( stderr, "ERROR: File is not a regular file nor a ");
                fprintf( stderr, "block device. Exit. ");
                return( -1);
            }  

            if( S_ISREG(st.st_mode)){
                if( st.st_size > 0){
                    options.size = (uint64_t) st.st_size;
                }
                options.flags |= MKFS_IS_REGULAR_FILE;
            }

            if( S_ISBLK(st.st_mode)){
                options.size = (uint64_t) get_bd_device_size( options.filename);
                options.flags |= MKFS_IS_BLOCKDEVICE;
            }
   
            options.flags |= OPT_SIZE;
        }
    }


    if( options.flags & (OPT_VERBOSE|OPT_CALCULATE)){
        printf("Passed arguments: \n");
        printf("    Filename=<%s>\n", options.filename);
        printf("    SSize=<%s>\n", options.str_size);
        printf("    SSInodesNum=<%s>\n", options.str_sinodes_num);
        printf("    SSlotsNum=<%s>\n", options.str_slots_num);
        printf("    SPercentage=<%s>\n", options.str_percentage);
        printf("    Size=<%lu>\n", options.size);
        printf("    SInodesNum=<%lu>\n", options.sinodes_num);
        printf("    SlotsNum=<%lu>\n", options.slots_num);
        printf("    Percentage=<%d>\n", options.percentage);
        printf("    Flags=<0x%x>\n", options.flags);
    }

    return(0);
}

int main( int argc, char **argv){
    int rc;

    /* parse options */
    rc = parse_opts( argc, argv);
    if( rc < 0){
        return( rc);
    }

    /* compute blocks and values */
    rc = compute_blocks();
    if( rc < 0){
        return( rc);
    }


    if( options.flags & (OPT_VERBOSE|OPT_CALCULATE) ){
        blocks_results_display();
    }

    if( options.flags & OPT_CALCULATE){
       exit( 0);
    }

    rc = build_filesystem_in_file(); 
    if( rc != 0){
        fprintf( stderr, "ERROR: Failed to build a filesystem\n");
    }else{
        printf("\nDone.\n");
    }

    return( rc);
}

