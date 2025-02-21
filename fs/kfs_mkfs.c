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
#include "kfs_config.h"
#include "eio.h"

#define CMD_KFS                                    0x000001
#define CMD_META                                   0x000002
#define CMD_OST                                    0x000004
#define CMD_GRAPH                                  0x000008
#define OPT_V                                      0x000010
#define OPT_H                                      0x000020
#define OPT_F                                      0x000040
#define OPT_D                                      0x000080
#define OPT_I                                      0x000100
#define OPT_S                                      0x000200
#define OPT_P                                      0x000400
#define OPT_W                                      0x000800
#define OPT_M                                      0x001000
#define OPT_K                                      0x002000
#define OPT_X_ITEMS                                0x004000
#define OPT_X_BLOCKS                               0x008000

#define MKFS_CREATE_FILE                           0x100000
#define MKFS_IS_BLOCKDEVICE                        0x200000
#define MKFS_IS_REGULAR_FILE                       0x400000

#define MKFS_DEFAULT_PERCENTAGE                    10


uint64_t blocks_per_block = KFS_BLOCKSIZE * 8;
typedef struct{
    char file_name[240];
    char conf_file[240];
    char metadata_file[240];
    char str_size[32];
    char str_slots_num[24];
    char str_sinodes_num[24];
    char str_percentage[10];
    char str_key[20];
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
    uint64_t out_sinodes_per_block, out_slots_per_block;
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
                                 } 


extern help_mkfs, help_mkfs_kfs, help_mkfs_meta, help_mkfs_ost;
extern help_mkfs_graph, help_mkfs_gennotes;

int display_help( char **s){
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
    sb->sb_file_header.kfs_magic = KFS_MAGIC;
    
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
    rc = block_write( fd, (void *) sb, 0);
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
    uint64_t i, block_num, sino_num;
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


    p = pages_alloc( 1);
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
    slp = p + sizeof( kfs_extent_header_t);

    sino = ( kfs_sinode_t *) slp;
    sino->si_a_time = current_time;
    sino->si_c_time = current_time;
    sino->si_m_time = current_time;
    sino->si_slot_id = 0xfffffffe;
    sino->si_id = sino_num++;

    /* fill in the other inodes */
    for( i = 1; i < bc->out_sinodes_per_block; i++){
        sino->si_id = sino_num++;
        sino++;
    }

    /* write the first block of the extent with the sinodes table */
    liminf = block_num = 1;
    block_write( fd, p, block_num++);
    limsup = liminf + bc->out_sinodes_table_in_blocks;

    /* write out the remaining blocks of the table */
    for( ; block_num < limsup; block_num++){
        memset( (void *) p, 0, KFS_BLOCKSIZE);
 
        i = 0;
        sino = ( kfs_sinode_t *) p;

        for( i = 0; i < bc->out_sinodes_per_block; i++){
            sino->si_id = sino_num++;
            sino++;
        }

        block_write( fd, p, block_num);
    }


    PRINTV("    -Written SuperInodes table blocks: [%lu-%lu]",
                liminf,
                limsup - 1); 
    PRINTV("    -Written SuperInodes num: [%lu, %lx]", sino_num, sino_num); 
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
        block_write( fd, slp, sinode_map_block + i);
        slp += KFS_BLOCKSIZE;
    }
    limsup = sinode_map_block + i - 1;
    PRINTV("    -Writing SuperInodes blocks map: [%lu-%lu]", 
                liminf, 
                limsup - 1);
    
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

    /* fill in the slots */
    slot_num = 0;
    slot = ( kfs_slot_t *) slp + sizeof( kfs_extent_header_t);
    for( i = 0; i < bc->out_slots_per_block; i++){
        slot->slot_id = slot_num++;
       slot++;
    }

    /* write the first block of the extent with the slots table */
    block_num = bc->out_sinodes_table_in_blocks + 1;
    liminf = block_num;
    block_write( fd, slp, block_num++);
    limsup = liminf + bc->out_slots_table_in_blocks;

    /* write out the remaining blocks of the table */
    for( ; block_num < limsup; block_num++){
        memset( (void *) slp, 0, KFS_BLOCKSIZE);
 
        i = 0;
        slot = ( kfs_slot_t *) p;

        for( i = 0; i < bc->out_slots_per_block; i++){
            slot->slot_id = slot_num++;
            slot++;
        }

        block_write( fd, p, block_num);
    }

    free( p);
    PRINTV("    -Written last slot num: [%lu, %lx]", slot_num, slot_num); 
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
        block_write( fd, slp, slot_map_block + i);
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
    kfs_extent_header_t *ex_header = (kfs_extent_header_t *) p;
    unsigned char *bitmap = (unsigned char *) p + 
                            sizeof( kfs_extent_header_t);
 
    PRINTV("-Writing block map");

    /* set maps addresses */
    fs_map_block = bc->in_file_size_in_blocks - bc->out_bitmap_size_in_blocks;
    block_num = bc->out_bitmap_size_in_blocks;

    /* mark the bits corresponding to the kfs block map */
    bm_set_extent( bitmap, 
                   bc->in_file_size_in_blocks, 
                   fs_map_block,
                   block_num,
                   1);
    ex_header->eh_entries_in_use += block_num;
 
    liminf = fs_map_block;
    for( i = 0; i < block_num; i++){
        block_write( fd, p, fs_map_block++);
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
    printf("    SlotsPerBlock=%lu, SinodesPerBlock=%lu\n", 
        bc->out_slots_per_block, bc->out_sinodes_per_block);

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
    uint64_t slots_blocks, slots_map_blocks;
    uint64_t sinodes_per_block;
    uint64_t sinodes_blocks, sinodes_map_blocks;
    uint64_t bits_per_block, rem, n;
    float factor = 1.0; /* divisor for both slots and super inodes */

    /* if we have metadata slots, super inodes and all in a single file*/
    if( options.flags & OPT_SINGLE){
        factor = 2.0;
    }

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
    slots_per_block = (KFS_BLOCKSIZE - sizeof( kfs_extent_header_t)) / 
                      sizeof( kfs_slot_t);
                      
    sinodes_per_block = (KFS_BLOCKSIZE - sizeof( kfs_extent_header_t)) /
                        sizeof( kfs_sinode_t);
 
    bc->out_slots_per_block = slots_per_block;
    bc->out_sinodes_per_block = sinodes_per_block;

    /* now calculate how many bits per block can our bitmap have */
    bits_per_block = (KFS_BLOCKSIZE - sizeof( kfs_extent_header_t)) * 8;



    /* add super block */
    if( options.flags & OPT_GRAPH){
        total_blocks_required++;
    }

    /* given the file/device size, and the percentage, calculate how
     * many super inodes and slots do we need */
    if( bc->in_sinodes_num == 0 && bc->in_slots_num == 0){

        /* calculate how many blocks for slots do we have */
        slots_blocks = (uint64_t) ( (float)bc->in_file_size_in_blocks * 
                        (( float) bc->in_percentage / 100.0) / factor);
        sinodes_blocks = slots_blocks;  /* same for sinodes table blocks */

        /* sum the total blocks required now */
        if( options.flags & OPT_META){
            total_blocks_required += slots_blocks;
        }else if( options.flags & OPT_OBJ){
            total_blocks_required += sinodes_blocks;
        }

        /* how many blocks our slots map need */
        rem = (( slots_blocks % bits_per_block) == 0 ) ? 0 : 1;
        slots_map_blocks = ( slots_blocks / bits_per_block) + rem;
                           
        /* real number of blocks for slots we have */
        slots_blocks -= slots_map_blocks;

        bc->out_slots_bitmap_blocks_num = slots_map_blocks;
        bc->out_slots_table_in_blocks = slots_blocks;
        bc->out_slots_num = slots_blocks * slots_per_block;

        /* calculate how many super inodes are available, from the file size 
         * in blocks  */
 
        /* how many blocks our super inodes map need */
        rem = (( sinodes_blocks % bits_per_block) == 0) ? 0 : 1;
        sinodes_map_blocks = ( sinodes_blocks / bits_per_block) + rem;

        /* real number of blocks for super inodes we have */
        sinodes_blocks -= sinodes_map_blocks;

        bc->out_sinodes_bitmap_blocks_num = sinodes_map_blocks;
        bc->out_sinodes_table_in_blocks = sinodes_blocks;
        bc->out_sinodes_num = sinodes_blocks * sinodes_per_block;
    }else{ /* we have a fixed number of super inodes and slots passed in
              from the CLI, so calculate how many blocks are required and 
              how big the file/device should  be. */
        if( bc->in_sinodes_num != 0){
            rem = (( bc->in_sinodes_num % sinodes_per_block) == 0) ? 0 : 1; 
            sinodes_blocks = ( bc->in_sinodes_num / sinodes_per_block) + rem;

            rem = (( sinodes_blocks % bits_per_block) == 0) ? 0 : 1; 
            sinodes_map_blocks = ( sinodes_blocks / bits_per_block) + rem;
            bc->out_sinodes_bitmap_blocks_num = sinodes_map_blocks;
            bc->out_sinodes_table_in_blocks = sinodes_blocks;
            total_blocks_required += sinodes_map_blocks + sinodes_blocks;
            bc->out_sinodes_num = sinodes_blocks * sinodes_per_block;

        }

        if( bc->in_slots_num != 0){
            rem = (( bc->in_slots_num % slots_per_block) == 0) ? 0 : 1;
            slots_blocks = ( bc->in_slots_num / slots_per_block) + rem;
            /* how many blocks our slots map need */
            rem = ( ( slots_blocks % bits_per_block) == 0) ? 0 : 1;
            slots_map_blocks = ( slots_blocks / bits_per_block) + rem;
            bc->out_slots_bitmap_blocks_num = slots_map_blocks;
            bc->out_slots_table_in_blocks = slots_blocks;
            total_blocks_required += slots_map_blocks + slots_blocks;
            bc->out_slots_num = slots_blocks * slots_per_block;
        }

        /* calculate total device size */
        n = total_blocks_required;
        bc->in_file_size_in_blocks = (uint64_t) ((float) n * 
                                        ( 100.0 / (float) bc->in_percentage));

        bc->in_file_size_in_mbytes = bc->in_file_size_in_blocks / 128;
   }



    /* now fill in the bitmap size in blocks and finish calculations */
    rem = ( (bc->in_file_size_in_blocks % bits_per_block) == 0 )? 0 : 1; 
    bitmap_size_in_blocks = (bc->in_file_size_in_blocks / bits_per_block) + 
                            rem;
    bc->out_bitmap_size_in_blocks = bitmap_size_in_blocks;
    total_blocks_required += bitmap_size_in_blocks;

    bc->out_total_blocks_required = total_blocks_required;

    return(0);
}


/* parse the passed options */
int parse_opts( int argc, char **argv){
    int opt, flags, mask; 
    struct stat st;
    int rc;
    int need_file = 1;
    char **options;
    char *command;

    flags = 0;
    char opc[] = "f:d:i:s:p:k:w:m:xvh";
    memset( (void *) &options, 0, sizeof( options_t));

    if( argc <= 1){
        TRACE_ERR( "invalid number of arguments.");
        display_help( help_mkfs);
        exit( EXIT_FAILURE);
    }      

    
    command = argv[1];
    if( strncmp(command, "kfs", 3) == 0){
        flags |= CMD_KFS;
    }else if( strncmp( command, "meta", 4) == 0){
        flags |= CMD_META;
    }else if( strncmp( command, "ost", 3) == 0){
        flags |= CMD_OST;
    }else if( strncmp( command, "graph", 5) == 0){
        flags |= CMD_GRAPH;
    }else{
        TRACE_ERR( "Invalid command '%s'", command);
        display_help( help_mkfs);
        exit( EXIT_FAILURE);
    }


    options = argv;
    options++;
    while ((opt = getopt(argc, argv, opc )) != -1) {
        switch (opt) {
            case 'f':
                flags |= OPT_F;
                strcpy( options.file_name, optarg);
                break;
            case 'd':
                flags |= OPT_D;
                strcpy( options.str_size, optarg);
                break;
            case 'i':
                flags |= OPT_I;
                strcpy( options.str_sinodes_num, optarg);
                break;
            case 's':
                flags |= OPT_S;
                strcpy( options.str_slots_num, optarg);
                break;
            case 'p':
                flags |= OPT_P;
                strcpy( options.str_percentage, optarg);
                break;
            case 'k':
                flags |= OPT_K;
                strcpy( options.str_key, optarg);
                break;
            case 'w':
                flags |= OPT_W;
                strcpy( options.conf_file, optarg);
                break;
            case 'm':
                flags |= OPT_M;
                strcpy( options.metadata_file, optarg);
                break;
            case 'x':
                if( strncmp( optarg, "blocks", 6) == 0){
                    flags |= OPT_X_BLOCKS;
                }if( strncmp( optarg, "items", 5) == 0){
                    flags |= OPT_X_ITEMS;
                }else{
                    TRACE_ERR( "Invalid option for -x argument");
                    display_help( help_mkfs);
                    exit( EXIT_FAILURE);
                }
                strcpy( options.metadata_file, optarg);
                break;
            case 'v':
                flags |= OPT_V;
                break;
            case 'h':
                flags |= OPT_H;
                break;
            default: /* '?' */
                TRACE_ERR( "invalid argument.");
                display_help( help_mkfs);
                exit(EXIT_FAILURE);
        }
    }

    /* if help required, display help and exit, no errors */
    if( flags & OPT_H){
        if( flags & CMD_KFS){
            display_help( help_mkfs_kfs );
        }else if( flags & CMD_META){

            display_help( help_mkfs_kfs );
        }
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
                    ( OPT_PERCENTAGE | OPT_NUM_SLOTS)) ||

         /* -g can not go with -i or -s */
         ( (flags & ( OPT_GRAPH | OPT_NUM_SLOTS)) ==
                    ( OPT_GRAPH | OPT_NUM_SLOTS)) ||
         ( (flags & ( OPT_GRAPH | OPT_NUM_SINODES)) ==
                    ( OPT_GRAPH | OPT_NUM_SINODES)) ||

         /* -o can not go with -s */
         ( (flags & ( OPT_OBJ | OPT_NUM_SLOTS)) ==
                    ( OPT_OBJ | OPT_NUM_SLOTS)) ||

         /* -m can not go with -i */
         ( (flags & ( OPT_META | OPT_NUM_SINODES)) ==
                    ( OPT_META | OPT_NUM_SINODES)) ||

         /* -m, -o or -g should go with -k, always */
         ( (flags & ( OPT_META | OPT_GRAPH | OPT_OBJ) != 0) && 
            (flags & OPT_KEY == 0)) 
        ){
        fprintf( stderr, "Invalid options combination.\n");
        display_help();
        exit( EXIT_FAILURE);
    }


    if( ( flags & ( OPT_META | OPT_OBJ | OPT_GRAPH)) == 0){
        flags |= (OPT_META | OPT_OBJ | OPT_GRAPH | OPT_SINGLE); /* a file system in a single file */
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
                options.size = (uint64_t) get_bd_size( options.filename);
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

