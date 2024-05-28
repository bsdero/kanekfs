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


int display_help(){
    char *help[]={
        "Usage: kfs_mkfs [options] file_name",
        "",
        "    Options:",
        "      -s fs_size        File size",
        "      -i sino_num       Number of Super Inodes",
        "      -n slot_num       Number of Metadata Slots", 
        "      -c                Calc number of items",
        "      -d                Dump details about file system",
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
        "      -If [-d] is specified, information about the filesystem is ",
        "       shown", 
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


char *page_alloc(){
    char *page;

    page = malloc( KFS_BLOCKSIZE);
    if( page == NULL){
        TRACE_SYSERR( "malloc error.\n");
        return( NULL);
    }

    /* Fill the whole file with zeroes */
    memset( page, 0, KFS_BLOCKSIZE);
    return( page);
}


int page_write( char *page, int fd, int block_num){
    lseek( fd, block_num * KFS_BLOCKSIZE, SEEK_SET);
    write( fd, page, KFS_BLOCKSIZE);

    return(0);
}

int build_filesystem_in_file( options_t *options, blocks_calc_t *bc){

#define KFS_ROOT_INODE_OFFSET          256
    int fd;
    uint64_t i;
    char *page, *sb_page;
    kfs_superblock_t sb; 
    /*mx_inode_t root_i;*/    
    time_t current_time;
    char secret[] = "Good! U found the secret message!! 1234567890";



    fd = open( options->filename, O_RDWR );
    if( fd < 0){
        perror( "ERROR: Could not open file\n");
        return( -1);
    }

    page = page_alloc();
    if( page == NULL){
        close( fd);
        exit(-1);
    }


    lseek( fd, 0, SEEK_SET);
    for( i = 0; i < bc->in_file_size_in_blocks; i++){
        write( fd, page, KFS_BLOCKSIZE);
        lseek( fd, 0, SEEK_END);
    }



    /* Build the super block */ 
    memset( (void *) &sb, 0, sizeof( kfs_superblock_t ));
    current_time = time( NULL);
    sb.sb_magic = KFS_MAGIC;
    sb.sb_version = 0x000001;
    sb.sb_flags = 0x0000;
    sb.sb_blocksize = KFS_BLOCKSIZE;
    sb.sb_root_super_inode = 0;
   

    sb.sb_c_time = current_time; 
    sb.sb_m_time = current_time; 
    sb.sb_a_time = current_time;

    

    /*
     *
    build_slots( kfs_superblock_t *sb, char *slots, char *slots_map);
    write_slots( kfs_superblock_t *sb, char *slots, char *slots_map);

    build_sinodes( kfs_superblock_t *sb, char *slots, char *slots_map);
    write_sinodes( kfs_superblock_t *sb, char *slots, char *slots_map);

    build_kfsmap( kfs_superblock_t *sb, char *slots, char *slots_map);
    write_kfsmap( kfs_superblock_t *sb, char *slots, char *slots_map);

    build_first_sinode()
    build_first_slot()

    write_superblock( kfs_superblock_t *sb);


    sync( fd);
    close( fd);

    kfs_mount( dev);



    sb.sb_slot_table.st_capacity = bc->out_slots_num;
    sb.sb_slot_table.st_in_use = 1;
    sb.sb_slot_table.st_table_extent. = 
   

    sb.sb_si_table.si_capacity = bc->out_sinodes_num;
    sb.sb_si_table.si_in_use = 1;



    sb.num_blocks = num_blocks; 
    sb.used_inodes = 1;
    sb.used_blocks = 2 + num_blocks_4_map; 
     blockmap + superblock + 
                                            * root inode data 
    sb.block_map = blockmap_address;
    sb.block_map_num = (uint64_t) num_blocks_4_map;
    current_time = time( NULL);
    sb.ctime = (uint64_t) current_time;
    sb.active_root_inode_block = 0;
    sb.active_root_inode_offset = KFS_ROOT_INODE_OFFSET;
    memset( &sb.spare_bytes, 'x', sizeof( sb.spare_bytes));
    strcpy( sb.spare_bytes, secret);

     build the root inode 
    memset( (void *) &root_i, 0, sizeof( root_i));
    root_i.a_time = root_i.c_time = root_i.m_time = current_time; 
    root_i.size = 0;
    root_i.flags = MX_STORAGE_IS_EXTENT; 
    root_i.num_blocks = 1; 
    root_i.mode = S_IFDIR | 666;
    root_i.num_links = 0;
    root_i.uid = 0;
    root_i.gid = 0;

     just 1 block for the root directory info, for start  
    root_i.storage.extents.extent[0].b_start = 1;
    root_i.storage.extents.extent[0].b_num = 1;


    copy the superblock and root inode data to the correct 
    place in the memory block 
    memset( p, 0, KFS_BLOCKSIZE);
    memcpy( p, &sb, sizeof( sb));
    memcpy( p + KFS_ROOT_INODE_OFFSET, &root_i, sizeof( root_i));

    write the whole block to disk 
    lseek( fd, 0, SEEK_SET);
    write( fd, p, KFS_BLOCKSIZE);

    printf(" -Wrote superblock and root inode\n");
    free( p);

    

    * almost there, except we need to prepare the block map also. We need to
     * mark block 0 as busy, as the superblock and root inode lives there.  
     * Also the block 1 needs to be marked, as the root inode has an extent 
     * pointing there. 
     * The blockmap used blocks should be marked also. *
    blockmap = malloc( KFS_BLOCKSIZE * num_blocks_4_map);
    if( blockmap == NULL){
        perror( "ERROR: malloc error.\n");
        close( fd);
        return( -1);
    }

    memset( blockmap, 0, KFS_BLOCKSIZE * num_blocks_4_map);

    * superblock and root inode *
    bm_set_bit( blockmap, num_blocks, (uint64_t) 0, 1);

    * superblock's data block *
    bm_set_bit( blockmap, num_blocks, (uint64_t) 1, 1);

    * used blocks map *
    bm_set_extent( blockmap, 
                   num_blocks, 
                   blockmap_address, 
                   num_blocks_4_map, 
                   1);

     
    lseek( fd, blockmap_address * KFS_BLOCKSIZE, SEEK_SET);
    write( fd, blockmap, KFS_BLOCKSIZE * num_blocks_4_map);
    
    close( fd);
    free( blockmap);
    printf(" -Wrote blockmap.\n");
    return(0);
    */
}

int blocks_calc_display( blocks_calc_t *bc){
    printf("IN:\n");
    printf("   InodesNum=%lu\n", bc->in_sinodes_num);
    printf("   SlotsNum=%lu\n", bc->in_slots_num);
    printf("   FileSizeInMB=%lu\n", bc->in_file_size_in_mbytes);
    printf("   FileSizeInBlocks=%lu\n", bc->in_file_size_in_blocks);
    printf("   Percentage=%d\n", bc->in_percentage);
    printf("OUT:\n");
    printf("   TotalBlocksRequired=%lu\n", bc->out_total_blocks_required);
    printf("   TotalDiskRequiredInMB=%lu\n",  /* 128 blocks per Mbyte */
           bc->out_total_blocks_required / 128);
    printf("   BitmapSizeInBlocks=%lu\n", bc->out_bitmap_size_in_blocks);

    printf("   InodesNum=%lu\n", bc->out_sinodes_num);
    printf("   SlotsNum=%lu\n", bc->out_slots_num);
    printf("   SlotsTableInBlocks=%lu, SlotsMapInBlocks=%lu\n", 
        bc->out_slots_table_in_blocks, bc->out_slots_bitmap_blocks_num);
    printf("   SinodesTableInBlocks=%lu, SinodesMapInBlocks=%lu\n", 
        bc->out_sinodes_table_in_blocks, bc->out_sinodes_bitmap_blocks_num);

    return(0);
}

int blocks_calc( blocks_calc_t *bc){
    uint64_t total_blocks_required = 0, bitmap_size_in_blocks = 0;
    uint64_t slots_per_block;
    uint64_t slots_blocks, slots_map_blocks, slots_result;
    uint64_t sinodes_per_block;
    uint64_t sinodes_blocks, sinodes_map_blocks, sinodes_result;

    uint64_t bits_per_block, n;

    n = KFS_BLOCKSIZE - sizeof( kfs_extent_header_t);
    slots_per_block = n / sizeof( kfs_slot_t);

    n = KFS_BLOCKSIZE - sizeof( kfs_extent_header_t);
    sinodes_per_block = n / sizeof( kfs_sinode_t);
 

    /* now calculate how many bits per block can have our bitmap */
    bits_per_block = (KFS_BLOCKSIZE - sizeof( kfs_extent_header_t)) * 8;


    if( bc->in_sinodes_num == 0 && bc->in_slots_num == 0){
        total_blocks_required += 1; /* super block */


        /* calculate how many slots ara available, from the file size 
         * in blocks  */

        /* calculate how many blocks for slots have we */
        slots_blocks = (uint64_t) ( (float)bc->in_file_size_in_blocks * 
                        (( float) bc->in_percentage / 100.0) / 2.0);
        sinodes_blocks = slots_blocks;  /* same for sinodes table blocks */

        /* sum the total blocks required now */
        total_blocks_required += slots_blocks + sinodes_blocks;

        TRACE("slots_blocks=%lu, sinodes_blocks=%lu\n", slots_blocks, sinodes_blocks);

        /* how many blocks our slots map need */
        slots_map_blocks = ( slots_blocks / bits_per_block) + 1;

        /* real number of blocks for slots we have */
        slots_blocks -= slots_map_blocks;

        /* now get the real slots number we can have */
        slots_result = slots_blocks * slots_per_block;


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
        sinodes_result = sinodes_blocks * sinodes_per_block;


        bc->out_sinodes_bitmap_blocks_num = sinodes_map_blocks;
        bc->out_sinodes_table_in_blocks = sinodes_blocks;
        bc->out_sinodes_num = sinodes_result;
    }else{
        if( bc->in_sinodes_num != 0){
            sinodes_blocks = ( bc->in_sinodes_num / 
                            sinodes_per_block) + 1;
            sinodes_map_blocks = ( sinodes_blocks / bits_per_block) + 1;
            sinodes_blocks -= sinodes_map_blocks;
            bc->out_sinodes_bitmap_blocks_num = sinodes_map_blocks;
            bc->out_sinodes_table_in_blocks = sinodes_blocks;
            total_blocks_required += sinodes_map_blocks + sinodes_blocks;
            bc->out_sinodes_num = bc->in_sinodes_num;
        }

        if( bc->in_slots_num != 0){
            slots_blocks = ( bc->in_slots_num  / 
                                slots_per_block) + 1;
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



    bitmap_size_in_blocks = (bc->in_file_size_in_blocks / bits_per_block) + 1;
    bc->out_bitmap_size_in_blocks = bitmap_size_in_blocks;
    total_blocks_required += bitmap_size_in_blocks;

    bc->out_total_blocks_required = total_blocks_required;

    return(0);
}

int parse_opts( int argc, char **argv, options_t *options){
    int opt, flags; 
    struct stat st;
    int rc;
    int need_file = 1;

#define OPT_SIZE                         0x0001
#define OPT_NUM_SLOTS                    0x0002
#define OPT_NUM_SINODES                  0x0004
#define OPT_CALCULATE                    0x0008
#define OPT_DUMP                         0x0010
#define OPT_PERCENTAGE                   0x0020
#define OPT_VERBOSE                      0x0040
#define OPT_HELP                         0x0080

#define MKFS_CREATE_FILE                 0x0100
#define MKFS_IS_BLOCKDEVICE              0x0200
#define MKFS_IS_REGULAR_FILE             0x0400

#define MKFS_DEFAULT_PERCENTAGE          10

    flags = 0;
    char opc[] = "s:i:n:cdp:vh";
    memset( (void *) options, 0, sizeof( options_t));

    if( argc == 0){
        fprintf(stderr, "ERROR: invalid number of arguments.\n");
        display_help();
        exit( EXIT_FAILURE);
    }      

    
    while ((opt = getopt(argc, argv, opc )) != -1) {
        switch (opt) {
            case 's':
                flags |= OPT_SIZE;
                strcpy( options->str_size, optarg);
                break;
            case 'i':
                flags |= OPT_NUM_SINODES;
                strcpy( options->str_sinodes_num, optarg);
                break;
            case 'n':
                flags |= OPT_NUM_SLOTS;
                strcpy( options->str_slots_num, optarg);
                break;
            case 'p':
                flags |= OPT_PERCENTAGE;
                strcpy( options->str_percentage, optarg);
                break;
            case 'd':
                flags |= OPT_DUMP;
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

         /* -d and -v with any extra flags */
         ( (flags & ( OPT_DUMP)) && ~( flags & (OPT_DUMP | OPT_VERBOSE))) || 
                                                                                                              
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
        strcpy( options->filename, argv[optind]);
    }
    options->flags = flags;

    if( options->flags & OPT_SIZE){
        options->size = validate_size( options->str_size);
        if( options->size == 0){
            fprintf( stderr, "ERROR: Size not valid\n");
            display_help();
            return( -1);
        }
        if( options->size < 20971520){
            fprintf( stderr, "ERROR: Invalid size, minimum is 20MBytes\n");
            return( -1);
        }
    }

   if( options->flags & OPT_PERCENTAGE){
        options->percentage = validate_num( options->str_percentage);
        if( options->percentage < 2 || options->percentage > 90 ){
            fprintf( stderr, "ERROR: Percentage not valid. Valid range is ");
            fprintf( stderr, "[2-90]\n");
            display_help();
            return( -1);
        }
    }else{
        options->percentage = MKFS_DEFAULT_PERCENTAGE;
    }



    if( options->flags & OPT_NUM_SINODES){
        options->sinodes_num = validate_num( options->str_sinodes_num);
        if( options->sinodes_num == 0){
            fprintf( stderr, "ERROR: SInodes num not valid\n");
            display_help();
            return( -1);
        }
    }
    if( options->flags & OPT_NUM_SLOTS){
        options->slots_num = validate_num( options->str_slots_num);
        if( options->slots_num == 0){
            fprintf( stderr, "ERROR: Slots num not valid\n");
            display_help();
            return( -1);
        }
    }


    
    if( need_file == 1){
        rc = stat( options->filename, &st);
        if( rc != 0){
            if( options->flags & OPT_DUMP){
                fprintf( stderr, "ERROR: File does not exist.\n");
                display_help();
                return( -1);
            }

            if( (options->flags & OPT_CALCULATE) == 0 ){
                printf("File '%s' does not exist, will create. \n", 
                        options->filename);
                options->flags |= MKFS_CREATE_FILE;
                options->flags |= MKFS_IS_REGULAR_FILE;
            }
        }else{
            if( (! S_ISREG(st.st_mode)) && (! S_ISBLK(st.st_mode))){
                fprintf( stderr, "ERROR: File is not a regular file nor a ");
                fprintf( stderr, "block device. Exit. ");
                return( -1);
            }  

            if( S_ISREG(st.st_mode)){
                if( st.st_size > 0){
                    options->size = (uint64_t) st.st_size;
                }
                options->flags |= MKFS_IS_REGULAR_FILE;
            }

            if( S_ISBLK(st.st_mode)){
                options->size = (uint64_t) get_bd_device_size( options->filename);
                options->flags |= MKFS_IS_BLOCKDEVICE;
            }
   
            options->flags |= OPT_SIZE;
        }
        printf(" -File system size: %lu Bytes (%lu MBytes, %.2f GBytes)\n",
            options->size, 
            options->size/_1M, 
            (double)options->size/(double)_1G);

    }


    if( options->flags & (OPT_VERBOSE|OPT_CALCULATE)){
        printf("Filename=<%s>\n", options->filename);
        printf("SSize=<%s>\n", options->str_size);
        printf("SSInodesNum=<%s>\n", options->str_sinodes_num);
        printf("SSlotsNum=<%s>\n", options->str_slots_num);
        printf("SPercentage=<%s>\n", options->str_percentage);
        printf("Size=<%lu>\n", options->size);
        printf("SInodesNum=<%lu>\n", options->sinodes_num);
        printf("SlotsNum=<%lu>\n", options->slots_num);
        printf("Percentage=<%d>\n", options->percentage);
        printf("Flags=<0x%x>\n", options->flags);
    }

    return(0);
}

int compute_blocks( options_t *options, blocks_calc_t *bc){
    int rc;

    memset( ( void *) bc, 0, sizeof( blocks_calc_t));
    bc->in_file_size_in_blocks = options->size / KFS_BLOCKSIZE;
    bc->in_file_size_in_mbytes = options->size / _1M;
 
    if( options->flags & (OPT_NUM_SINODES|OPT_NUM_SLOTS)){
        
        bc->in_sinodes_num = options->sinodes_num; 
        bc->in_slots_num = options->slots_num;
    }
    

    bc->in_percentage = options->percentage;
    rc = blocks_calc( bc);
    return( rc);
}

int main( int argc, char **argv){
    int rc;
    options_t options;
    blocks_calc_t blocks_results;


    /* parse options */
    rc = parse_opts( argc, argv, &options);

    /* compute blocks and values */
    rc = compute_blocks( &options, &blocks_results);

    if( options.flags & (OPT_VERBOSE|OPT_CALCULATE) ){
        blocks_calc_display( &blocks_results);
    }

    if( options.flags & OPT_CALCULATE){
       exit( 0);
    }

 
    rc = build_filesystem_in_file( &options, &blocks_results); 
    if( rc != 0){
        fprintf( stderr, "ERROR: Failed to build a filesystem\n");
    }else{
        printf("\nDone.\n");
    }

    return( rc);
}

