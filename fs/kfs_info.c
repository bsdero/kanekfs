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


int main( int argc, char **argv){
    char filename[256], buff[256];
    uint64_t size, addr;
    struct stat st;
    int rc, fd, i;
    char *p, *pex;
    kfs_superblock_t *sb;
    kfs_extent_t *e;
    kfs_extent_header_t *eh;
    kfs_sinode_t *si;
    kfs_slot_t *slot;
    time_t ctime, atime, mtime;

    if( argc != 2 ){
        printf("ERROR: Incorrect arguments number\n");
        printf("Usage: \n");
        printf("    kfs_info <filename>\n");
        printf("\nkfs_info displays kfs filesystem information\n");
        printf("---------------------------------------------------------\n");
        return( -1);
    }
    
    strncpy( filename, argv[1], 254);
    printf("filename=%s\n", filename);

    rc = stat( filename, &st);
    if( rc != 0){
        TRACE_ERR("File does not exist");
        return( -1);
    }

    if( (! S_ISREG(st.st_mode)) && (! S_ISBLK(st.st_mode))){
        TRACE_ERR( "File is not a regular file nor a block device. Exit. ");
        return( -1);
    }  

    if( S_ISREG(st.st_mode)){
        size = (uint64_t) st.st_size;
    }else if( S_ISBLK(st.st_mode)){
        size = (uint64_t) get_bd_device_size( filename);
    }
     
    printf("size=%lu\n", size);
    

    p = malloc( KFS_BLOCKSIZE);
    if( p == NULL){
        TRACE_ERR("Could not reserve memory. Exit.");
        return( -1);
    }


    fd = open( filename, O_RDONLY);
    if( fd < 0){
        TRACE_ERR("Could not open block device.");
        return( -1);
    }
 
    lseek( fd, 0, SEEK_SET);
    read( fd, p, KFS_BLOCKSIZE);


    sb = (kfs_superblock_t *) p;

    if( sb->sb_magic != KFS_MAGIC){
        TRACE_ERR("Not a KFS file system. Abort.");
        return( -1);
    }

    printf("KFS Magic: 0x%lx\n", sb->sb_magic);
    printf("KFS Version: %u\n", sb->sb_version);
    printf("KFS Flags: 0x%x\n", sb->sb_flags);
    printf("KFS Blocksize: %lu\n", sb->sb_blocksize);
    printf("KFS root super inode: %lu\n", sb->sb_root_super_inode);

    ctime = sb->sb_c_time;
    atime = sb->sb_a_time;
    mtime = sb->sb_m_time;

    strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&ctime));
    printf("KFS Ctime: %s\n", buff); 

    strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&atime));    
    printf("KFS Atime: %s\n", buff); 

    strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&mtime));
    printf("KFS Mtime: %s\n", buff); 

    printf("SUPER INODES:\n");
    printf("    NumberOfSuperInodes: %lu\n", sb->sb_si_table.capacity );
    printf("    SuperInodesInUse: %lu\n", sb->sb_si_table.in_use ); 
    printf("    SuperInodesTableAddress: %lu\n",  
                sb->sb_si_table.table_extent.ee_block_addr );
    printf("    SuperInodesTableBlocksNum: %u\n",  
                sb->sb_si_table.table_extent.ee_block_size );
    printf("    SuperInodesMapAddress: %lu\n",  
                sb->sb_si_table.bitmap_extent.ee_block_addr );
    printf("    SuperInodesMapBlocksNum: %u\n",  
                sb->sb_si_table.bitmap_extent.ee_block_size );

    printf("SLOTS:\n");
    printf("    NumberOfSlots: %lu\n", sb->sb_slot_table.capacity );
    printf("    SlotsInUse: %lu\n", sb->sb_slot_table.in_use ); 
    printf("    SlotsTableAddress: %lu\n",  
                sb->sb_slot_table.table_extent.ee_block_addr );
    printf("    SlotsTableBlocksNum: %u\n",  
                sb->sb_slot_table.table_extent.ee_block_size );
    printf("    SlotsMapAddress: %lu\n",  
                sb->sb_slot_table.bitmap_extent.ee_block_addr );
    printf("    SlotsMapBlocksNum: %u\n",  
                sb->sb_slot_table.bitmap_extent.ee_block_size );

 

    printf("KFS Bitmap:\n");
    printf("    KFSBlockMapAddress: %lu\n",  
                sb->sb_blockmap.bitmap_extent.ee_block_addr );
    printf("    KFSBlockMapBlocksNum: %u\n",  
                sb->sb_blockmap.bitmap_extent.ee_block_size );
    
    printf("KFS BlocksRange:\n");
    printf("    SuperBlock: [0]\n");

    e = &sb->sb_si_table.table_extent;
    printf("    SuperInodesTable: [%lu-%lu]\n",
           e->ee_block_addr,
           e->ee_block_addr + e->ee_block_size - 1);

    e = &sb->sb_slot_table.table_extent;
    printf("    SlotsTable: [%lu-%lu]\n",
           e->ee_block_addr,
           e->ee_block_addr + e->ee_block_size - 1);
           
    e = &sb->sb_si_table.bitmap_extent;
    printf("    SuperInodesMap: [%lu-%lu]\n",
           e->ee_block_addr,
           e->ee_block_addr + e->ee_block_size - 1);

    e = &sb->sb_slot_table.bitmap_extent;
    printf("    SlotsMap: [%lu-%lu]\n",
           e->ee_block_addr,
           e->ee_block_addr + e->ee_block_size - 1);

    e = &sb->sb_blockmap.bitmap_extent;
    printf("    KFSBlockMap: [%lu-%lu]\n",
           e->ee_block_addr,
           e->ee_block_addr + e->ee_block_size - 1);




    pex = malloc( KFS_BLOCKSIZE);
    if( pex == NULL){
        TRACE_ERR("Could not reserve memory. Exit.");
        return( -1);
    }

    printf("\nChecking KFS Tables extents\n");
    addr = sb->sb_si_table.table_extent.ee_block_addr;
    printf("-Reading Super Inodes Table Extent in: %lu\n", addr); 
    lseek( fd, addr * KFS_BLOCKSIZE, SEEK_SET);
    read( fd, pex, KFS_BLOCKSIZE);
    eh = (kfs_extent_header_t *) pex;

    
    if( eh->eh_magic != KFS_SINODE_TABLE_MAGIC){
        TRACE_ERR("KFS super inode table not detected. Abort.");
        return( -1);
    }
    
    printf("-SuperInodesTable extent\n");
    printf("    -Magic : 0x%x\n", eh->eh_magic);
    printf("    -SuperInodesCapacity: %d\n", eh->eh_entries_capacity );
    printf("    -SuperInodesInUse: %d\n", eh->eh_entries_in_use );
    printf("    -Flags: 0x%x\n", eh->eh_flags);

    addr = sb->sb_si_table.table_extent.ee_block_addr + 
           sb->sb_si_table.table_extent.ee_block_size - 1;

    printf("    -Verifying last block of super inode extent in: %lu\n", addr); 
    lseek( fd, addr * KFS_BLOCKSIZE, SEEK_SET);
    read( fd, pex, KFS_BLOCKSIZE);

    si = ( kfs_sinode_t *) pex;
    i = 0;
    while( si->si_id < sb->sb_si_table.capacity-1){
       if( i >= KFS_BLOCKSIZE){
            TRACE_ERR("Unexpected, the super inode ID is beyond the page.");
            TRACE_ERR("last inode: [%lu-0x%lx]", si->si_id, si->si_id);
            exit(-1);
        }
        si++;
        i += sizeof( kfs_sinode_t);
    }
    printf("    -All seems to be fine, last inode: [%lu-0x%lx]\n", 
                si->si_id, si->si_id);




    addr = sb->sb_slot_table.table_extent.ee_block_addr;
    printf("-Reading Slot Table Extent in: %lu\n", addr); 
    lseek( fd, addr * KFS_BLOCKSIZE, SEEK_SET);
    read( fd, pex, KFS_BLOCKSIZE);
    eh = (kfs_extent_header_t *) pex;

    
    if( eh->eh_magic != KFS_SLOTS_TABLE_MAGIC){
        TRACE_ERR("KFS slots table not detected. Abort.");
        return( -1);
    }
    
    printf("-SlotsTable extent\n");
    printf("    -Magic : 0x%x\n", eh->eh_magic);
    printf("    -SlotsCapacity: %d\n", eh->eh_entries_capacity );
    printf("    -SlotsInUse: %d\n", eh->eh_entries_in_use );
    printf("    -Flags: 0x%x\n", eh->eh_flags);

    addr = sb->sb_slot_table.table_extent.ee_block_addr + 
           sb->sb_slot_table.table_extent.ee_block_size - 1;

    printf("     Verifying last block of slot extent in: %lu\n", addr); 
    lseek( fd, addr * KFS_BLOCKSIZE, SEEK_SET);
    read( fd, pex, KFS_BLOCKSIZE);

    slot = ( kfs_slot_t *) pex;
    i = 0;
    while( slot->slot_id < sb->sb_slot_table.capacity-1){
       if( i >= KFS_BLOCKSIZE){
            TRACE_ERR("Unexpected, the slot ID is beyond the page.");
            TRACE_ERR("last inode: [%u-0x%x]", 
                    slot->slot_id, slot->slot_id);
            exit(-1);
        }
        slot++;
        i += sizeof( kfs_slot_t);
    }
   
    printf("    -All seems to be fine, last slot: [%u-0x%x]\n", 
                 slot->slot_id, slot->slot_id);


    printf("\nChecking Map extents\n");
    
    addr = sb->sb_si_table.bitmap_extent.ee_block_addr;
    printf("-Reading Super Inode Table Map Extent in: %lu\n", addr); 
    lseek( fd, addr * KFS_BLOCKSIZE, SEEK_SET);
    read( fd, pex, KFS_BLOCKSIZE);
    eh = (kfs_extent_header_t *) pex;
  
    if( eh->eh_magic != KFS_SINODE_BITMAP_MAGIC){
        TRACE_ERR("KFS super inode bitmap table not detected. Abort.");
        return( -1);
    }
    
    printf("-SuperInodesTable Bitmap extent\n");
    printf("    -Magic : 0x%x\n", eh->eh_magic);
    printf("    -SuperInodesCapacity: %d\n", eh->eh_entries_capacity );
    printf("    -SuperInodesInUse: %d\n", eh->eh_entries_in_use );
    printf("    -Flags: 0x%x\n", eh->eh_flags);


    addr = sb->sb_slot_table.bitmap_extent.ee_block_addr;
    printf("-Reading Slot Table Map Extent in: %lu\n", addr); 
    lseek( fd, addr * KFS_BLOCKSIZE, SEEK_SET);
    read( fd, pex, KFS_BLOCKSIZE);
    eh = (kfs_extent_header_t *) pex;
  
    if( eh->eh_magic != KFS_SLOTS_BITMAP_MAGIC){
        TRACE_ERR("KFS slots bitmap table not detected. Abort.");
        return( -1);
    }
    
    printf("-SlotsTable Bitmap extent\n");
    printf("    -Magic : 0x%x\n", eh->eh_magic);
    printf("    -SlotsCapacity: %d\n", eh->eh_entries_capacity );
    printf("    -SlotsInUse: %d\n", eh->eh_entries_in_use );
    printf("    -Flags: 0x%x\n", eh->eh_flags);

    addr = sb->sb_blockmap.bitmap_extent.ee_block_addr;
    printf("-Reading KFS BlockMap Extent in: %lu\n", addr); 
    lseek( fd, addr * KFS_BLOCKSIZE, SEEK_SET);
    read( fd, pex, KFS_BLOCKSIZE);
    eh = (kfs_extent_header_t *) pex;
  
    if( eh->eh_magic != KFS_BLOCKMAP_MAGIC){
        TRACE_ERR("KFS slots bitmap table not detected. Abort.");
        return( -1);
    }
    
    printf("-SlotsTable Bitmap extent\n");
    printf("    -Magic : 0x%x\n", eh->eh_magic);
    printf("    -BlocksCapacity: %d\n", eh->eh_entries_capacity );
    printf("    -BlocksInUse: %d\n", eh->eh_entries_in_use );
    printf("    -Flags: 0x%x\n", eh->eh_flags);

 
    close( fd);
    free(p);
    free(pex);
    return(0);
}


