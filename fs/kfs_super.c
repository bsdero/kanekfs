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
#include "eio.h"
#include "page_cache.h"



/* super block methods */
sb_t __sb = { 0};

/* convert from kfs_sb ( KFS super block on disk to KFS superblock in 
 * memory, and vice versa */
void kfssb_to_sb( sb_t *sb, kfs_superblock_t *kfs_sb);
void sb_to_kfssb( kfs_superblock_t *ksb, sb_t *sb);

/* convert from extents on disk, to extents on memory and vice versa */
void ex_2_kfsex( kfs_extent_t *kex, extent_t *ex){
    kex->ee_block_addr = ex->ex_block_addr;
    kex->ee_block_size = ex->ex_block_size;
    kex->ee_log_size = ex->ex_log_size;
    kex->ee_log_addr = ex->ex_log_addr;
}

void kfsex_2_ex( extent_t *ex, kfs_extent_t *kex){
    ex->ex_block_addr = kex->ee_block_addr;
    ex->ex_block_size = kex->ee_block_size;
    ex->ex_log_size = kex->ee_log_size;
    ex->ex_log_addr = kex->ee_log_addr;
}


/*
void kfs_sb_statfs(){
    kfs_sb_t sb = __kfs_sb;
    char disk_type[32];

    printf("KanekFS, A graph file system\n");
    printf("   Root super inode id=%ul\n", sb.sb_root_super_inode );
    printf("   Super inodes capacity=%ul\n", sb.sb_sinodes.t_capacity );
    printf("   Super inodes in use=%ul\n", sb.sb_sinodes.t_in_use );
    printf("   Slots capacity=%ul\n", sb.sb_slots.t_capacity );
    printf("   Slots in use=%ul\n", sb.sb_slots.t_in_use );
}

*/

/* open a kfs_filesystem */ 
int kfs_open( kfs_config_t *config){
    int rc, fd; 

    rc = kfs_verify( config->kfs_file, 0, 0);
    if( rc < 0){
        TRACE_ERR("Verification failed, abort");
        return( rc);
    }

    fd = open( config->kfs_file, O_RDWR );
    if( fd < 0){
        TRACE_ERR( "ERROR: Could not open file '%s'\n", config->kfs_file);
    }

    return( fd);
}

/* verify the kfs superblock is valid */
int kfs_verify( char *filename, int verbose, int extra_verification){
    char buff[256];
    uint64_t size, addr, last_ino, sipp, slpp;
    struct stat st;
    int rc, fd;
    char *p, *pex;
    kfs_superblock_t *sb;
    kfs_extent_t *e;
    kfs_extent_header_t *eh;
    kfs_sinode_t *si;
    kfs_slot_t *slot;
    time_t ctime, atime, mtime;

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
        size = (uint64_t) get_bd_size( filename);
    }
     

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

    block_read( fd, p, 0);

    sb = (kfs_superblock_t *) p;

    if( sb->sb_magic != KFS_MAGIC){
        TRACE_ERR("Not a KFS file system. Abort.");
        return( -1);
    }
    ctime = sb->sb_c_time;
    atime = sb->sb_a_time;
    mtime = sb->sb_m_time;


    if( verbose){
        printf("File Size: %lu MBytes\n", size / _1M);
        printf("KFS Magic: 0x%lx\n", sb->sb_magic);
        printf("KFS Flags: 0x%x\n", sb->sb_flags);
        printf("KFS Blocksize: %lu\n", sb->sb_blocksize);
        printf("KFS root super inode: %lu\n", sb->sb_root_super_inode);

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
    }

    if( extra_verification == 0){
        close( fd);
        free(p);

        return(0);
    }

    pex = malloc( KFS_BLOCKSIZE);
    if( pex == NULL){
        TRACE_ERR("Could not reserve memory. Exit.");
        return( -1);
    }

    addr = sb->sb_si_table.table_extent.ee_block_addr;
    if( verbose){
        printf("\nChecking KFS Tables extents\n");
        printf("-Reading Super Inodes Table Extent in: %lu\n", addr); 
    } 
   

    block_read( fd, pex, addr);
    eh = (kfs_extent_header_t *) pex;

    if( eh->eh_magic != KFS_SINODE_TABLE_MAGIC){
        TRACE_ERR("KFS super inode table not detected. Abort.");
        return( -1);
    }
 
    if( verbose){
        printf("-SuperInodesTable extent\n");
        printf("    -Magic : 0x%x\n", eh->eh_magic);
        printf("    -SuperInodesCapacity: %d\n", eh->eh_entries_capacity );
        printf("    -SuperInodesInUse: %d\n", eh->eh_entries_in_use );
        printf("    -Flags: 0x%x\n", eh->eh_flags);
    }
    addr = sb->sb_si_table.table_extent.ee_block_addr + 
           sb->sb_si_table.table_extent.ee_block_size - 1;

    if( verbose){
        printf("    -Verifying last block of super inode extent in: %lu\n", 
                addr);
    }

    block_read( fd, pex, addr);

    sipp = (KFS_BLOCKSIZE / sizeof( kfs_sinode_t));

    /* get last inode */
    si = ( kfs_sinode_t *) pex;
    si = &si[sipp - 1];

    last_ino = si->si_id;

    if( last_ino != sb->sb_si_table.capacity-1){
        TRACE_ERR("Unexpected, the super inode ID is not fine");
        TRACE_ERR("last inode: [%lu-0x%lx]", last_ino, last_ino);
        return(-1);
    }
    if( verbose){
        printf("    -All seems to be fine, last inode: [%lu-0x%lx]\n", 
                    si->si_id, si->si_id);
    }



    addr = sb->sb_slot_table.table_extent.ee_block_addr;
    if( verbose){
        printf("-Reading Slot Table Extent in: %lu\n", addr); 
    }
    block_read( fd, pex, addr);
    eh = (kfs_extent_header_t *) pex;

    
    if( eh->eh_magic != KFS_SLOTS_TABLE_MAGIC){
        TRACE_ERR("KFS slots table not detected. Abort.");
        return( -1);
    }
    
    if( verbose){
        printf("-SlotsTable extent\n");
        printf("    -Magic : 0x%x\n", eh->eh_magic);
        printf("    -SlotsCapacity: %d\n", eh->eh_entries_capacity );
        printf("    -SlotsInUse: %d\n", eh->eh_entries_in_use );
        printf("    -Flags: 0x%x\n", eh->eh_flags);
    }
    addr = sb->sb_slot_table.table_extent.ee_block_addr + 
           sb->sb_slot_table.table_extent.ee_block_size - 1;

    if( verbose){
        printf("     Verifying last block of slot extent in: %lu\n", addr); 
    }
    block_read( fd, pex, addr);

    slpp = (KFS_BLOCKSIZE / sizeof( kfs_slot_t));

    /* get last inode */
    slot = ( kfs_slot_t *) pex;
    slot = &slot[slpp - 1];

    if( slot->slot_id != sb->sb_slot_table.capacity-1){
        TRACE_ERR("Unexpected, the slot ID is not fine.");
        TRACE_ERR("last slot: [%lu-0x%lx]", 
                    slot->slot_id, slot->slot_id);
            return(-1);
    }
   
    if( verbose){
        printf("    -All seems to be fine, last slot: [%lu-0x%lx]\n", 
                     slot->slot_id, slot->slot_id);
    }


    if( verbose){
        printf("Checking Map extents\n");
    }

    addr = sb->sb_si_table.bitmap_extent.ee_block_addr;
    if( verbose){
        printf("-Reading Super Inode Table Map Extent in: %lu\n", addr); 
    }

    block_read( fd, pex, addr);
    eh = (kfs_extent_header_t *) pex;
  
    if( eh->eh_magic != KFS_SINODE_BITMAP_MAGIC){
        TRACE_ERR("KFS super inode bitmap table not detected. Abort.");
        return( -1);
    }
    

    if( verbose){
        printf("-SuperInodesTable Bitmap extent\n");
        printf("    -Magic : 0x%x\n", eh->eh_magic);
        printf("    -SuperInodesCapacity: %d\n", eh->eh_entries_capacity );
        printf("    -SuperInodesInUse: %d\n", eh->eh_entries_in_use );
        printf("    -Flags: 0x%x\n", eh->eh_flags);
    }

    addr = sb->sb_slot_table.bitmap_extent.ee_block_addr;

    if( verbose){ 
        printf("-Reading Slot Table Map Extent in: %lu\n", addr); 
    }
    block_read( fd, pex, addr);
    eh = (kfs_extent_header_t *) pex;
  
    if( eh->eh_magic != KFS_SLOTS_BITMAP_MAGIC){
        TRACE_ERR("KFS slots bitmap table not detected. Abort.");
        return( -1);
    }
    

    if( verbose){
        printf("-SlotsTable Bitmap extent\n");
        printf("    -Magic : 0x%x\n", eh->eh_magic);
        printf("    -SlotsCapacity: %d\n", eh->eh_entries_capacity );
        printf("    -SlotsInUse: %d\n", eh->eh_entries_in_use );
        printf("    -Flags: 0x%x\n", eh->eh_flags);
    }
    addr = sb->sb_blockmap.bitmap_extent.ee_block_addr;

    if( verbose){
        printf("-Reading KFS BlockMap Extent in: %lu\n", addr); 
    }
    block_read( fd, pex, addr);
    eh = (kfs_extent_header_t *) pex;
  
    if( eh->eh_magic != KFS_BLOCKMAP_MAGIC){
        TRACE_ERR("KFS slots bitmap table not detected. Abort.");
        return( -1);
    }
    
    if( verbose){
        printf("-SlotsTable Bitmap extent\n");
        printf("    -Magic : 0x%x\n", eh->eh_magic);
        printf("    -BlocksCapacity: %d\n", eh->eh_entries_capacity );
        printf("    -BlocksInUse: %d\n", eh->eh_entries_in_use );
        printf("    -Flags: 0x%x\n", eh->eh_flags);
    }
    free( pex);


    close( fd);
    free(p);
    return(0);
}

/* read super block from a memory block */
void kfssb_to_sb( sb_t *sb, kfs_superblock_t *kfs_sb){
    kfs_extent_t *kex;
    extent_t *ex;
     
    memset( sb, 0, sizeof( sb_t));

    sb->sb_root_super_inode = kfs_sb->sb_root_super_inode;
    sb->sb_flags = kfs_sb->sb_flags;
    sb->sb_c_time = kfs_sb->sb_c_time;
    sb->sb_m_time = kfs_sb->sb_m_time;
    sb->sb_a_time = kfs_sb->sb_a_time;
    sb->sb_magic = kfs_sb->sb_magic;
    sb->sb_si_table.capacity = kfs_sb->sb_si_table.capacity;
    sb->sb_si_table.in_use = kfs_sb->sb_si_table.in_use;
    kex = &kfs_sb->sb_si_table.table_extent;
    ex = &sb->sb_si_table.table_extent;
    kfsex_2_ex( ex, kex);
    kex = &kfs_sb->sb_si_table.bitmap_extent;
    ex = &sb->sb_si_table.bitmap_extent;
    kfsex_2_ex( ex, kex);

    sb->sb_slot_table.capacity = kfs_sb->sb_slot_table.capacity;
    sb->sb_slot_table.in_use = kfs_sb->sb_slot_table.in_use;
    kex = &kfs_sb->sb_slot_table.table_extent;
    ex = &sb->sb_slot_table.table_extent;
    kfsex_2_ex( ex, kex);
    kex = &kfs_sb->sb_slot_table.bitmap_extent;
    ex = &sb->sb_slot_table.bitmap_extent;
    kfsex_2_ex( ex, kex);


    sb->sb_blockmap.capacity = kfs_sb->sb_blockmap.capacity;
    sb->sb_blockmap.in_use = kfs_sb->sb_blockmap.in_use;
    kex = &kfs_sb->sb_blockmap.table_extent;
    ex = &sb->sb_blockmap.table_extent;
    kfsex_2_ex( ex, kex);
    kex = &kfs_sb->sb_blockmap.bitmap_extent;
    ex = &sb->sb_blockmap.bitmap_extent;
    kfsex_2_ex( ex, kex);

}

/* to mount the super block implies to create a page cache for deal with
 * it. So, once the super block is mounted, all the IO should be done thru
 * the page cache */
int kfs_mount( kfs_config_t *config){
    int rc = 0, fd;
    pgcache_t *pgcache;
    pgcache_element_t *el;
    kfs_superblock_t *kfs_sb;
    sb_t *sb = &__sb;
    time_t now;

    fd = kfs_open( config);
    if( fd < 0){
        TRACE_ERR("kfs_open() failed! abort.");
        rc = -1;
        goto exit1; 
    }

    pgcache = pgcache_alloc( fd, config->cache_page_len);
    if( pgcache == NULL){
        TRACE_ERR("Issues in kfs_pgcache_alloc()");
        rc = -1;
        goto exit1;
    }


    rc = pgcache_enable_sync( pgcache );
    if( rc != 0){
        TRACE_ERR("Issues in pgcache_enable_sync()");
        rc = -1;
        goto exit0;
    }

    el = pgcache_element_map_sync( pgcache, 0, 1);
    if( el == NULL){
        TRACE_ERR("Issues in pgcache_element_map_sync()");
        rc = -1;
        goto exit0;
    }

    kfs_sb = (kfs_superblock_t *) el->pe_mem_ptr;
    kfssb_to_sb( sb, kfs_sb);

    sb->sb_bdev = fd;
    sb->sb_page_cache = (void *) pgcache; 
    sb->sb_superblock_page = (void *) el;
    sb->sb_flags = kfs_sb->sb_flags |= KFS_IS_MOUNTED;

    now = time(NULL);
    sb->sb_m_time = sb->sb_a_time = now;
    kfs_sb->sb_m_time = kfs_sb->sb_a_time = now;


    rc = pgcache_element_sync( el);
    if( rc != 0){
        TRACE_ERR("could not sync cache_element");
        goto exit0;
    }

    if( kfs_active() != 0 ){
        TRACE_ERR("Superblock is not active");
        goto exit0;
    }
 
    goto exitOK;

exit0:
    pgcache_destroy( pgcache);

exit1:
    close( fd);
exitOK:

    return( rc);
}


int kfs_active(){
    sb_t *sb = &__sb;
    pgcache_t *pgcache;
 
    if( sb->sb_magic != KFS_MAGIC){
        TRACE_ERR("Magic does not match");
        return( -1);
    }

    
    if( ( sb->sb_flags & KFS_IS_MOUNTED) == 0){
        TRACE_ERR("KFS_IS_MOUNTED flag is not enabled");
        return( -1);
    }

    pgcache = (pgcache_t *) sb->sb_page_cache;


    if( ! IS_CACHE_ACTIVE( pgcache)){
        TRACE_ERR("Page Cache is not active");
        return( -1);
    }
 
    return(0);
}

void kfs_superblock_display(){
    sb_t *sb = &__sb;
    char buff[240];
    extent_t *e;

    TRACE("start");
    if( kfs_active() != 0 ){
        TRACE_ERR("Superblock is not active");
        return;
    }

    printf("SuperBlock\n");
    printf("KFS Flags: 0x%lx\n", sb->sb_flags);
    printf("KFS root super inode: %lu\n", sb->sb_root_super_inode);

    strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime( &sb->sb_c_time));
    printf("KFS Ctime: %s\n", buff); 

    strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime( &sb->sb_a_time));    
    printf("KFS Atime: %s\n", buff); 

    strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime( &sb->sb_m_time));
    printf("KFS Mtime: %s\n", buff); 

    printf("SUPER INODES:\n");
    printf("    NumberOfSuperInodes: %lu\n", sb->sb_si_table.capacity );
    printf("    SuperInodesInUse: %lu\n", sb->sb_si_table.in_use ); 
    printf("    SuperInodesTableAddress: %lu\n",  
                sb->sb_si_table.table_extent.ex_block_addr );
    printf("    SuperInodesTableBlocksNum: %u\n",  
                sb->sb_si_table.table_extent.ex_block_size );
    printf("    SuperInodesMapAddress: %lu\n",  
                sb->sb_si_table.bitmap_extent.ex_block_addr );
    printf("    SuperInodesMapBlocksNum: %u\n",  
                sb->sb_si_table.bitmap_extent.ex_block_size );

    printf("SLOTS:\n");
    printf("    NumberOfSlots: %lu\n", sb->sb_slot_table.capacity );
    printf("    SlotsInUse: %lu\n", sb->sb_slot_table.in_use ); 
    printf("    SlotsTableAddress: %lu\n",  
                sb->sb_slot_table.table_extent.ex_block_addr );
    printf("    SlotsTableBlocksNum: %u\n",  
                sb->sb_slot_table.table_extent.ex_block_size );
    printf("    SlotsMapAddress: %lu\n",  
                sb->sb_slot_table.bitmap_extent.ex_block_addr );
    printf("    SlotsMapBlocksNum: %u\n",  
                sb->sb_slot_table.bitmap_extent.ex_block_size );

    printf("KFS Bitmap:\n");
    printf("    KFSBlockMapAddress: %lu\n",  
                sb->sb_blockmap.bitmap_extent.ex_block_addr );
    printf("    KFSBlockMapBlocksNum: %u\n",  
                sb->sb_blockmap.bitmap_extent.ex_block_size );
        
    printf("KFS BlocksRange:\n");
    printf("    SuperBlock: [0]\n");

    e = &sb->sb_si_table.table_extent;
    printf("    SuperInodesTable: [%lu-%lu]\n",
           e->ex_block_addr,
           e->ex_block_addr + e->ex_block_size - 1);

    e = &sb->sb_slot_table.table_extent;
    printf("    SlotsTable: [%lu-%lu]\n",
           e->ex_block_addr,
           e->ex_block_addr + e->ex_block_size - 1);
               
    e = &sb->sb_si_table.bitmap_extent;
    printf("    SuperInodesMap: [%lu-%lu]\n",
           e->ex_block_addr,
           e->ex_block_addr + e->ex_block_size - 1);

    e = &sb->sb_slot_table.bitmap_extent;
    printf("    SlotsMap: [%lu-%lu]\n",
           e->ex_block_addr,
           e->ex_block_addr + e->ex_block_size - 1);

    e = &sb->sb_blockmap.bitmap_extent;
    printf("    KFSBlockMap: [%lu-%lu]\n",
           e->ex_block_addr,
           e->ex_block_addr + e->ex_block_size - 1);
    TRACE("end");
}


int kfs_umount(){
    sb_t *sb = &__sb;
    int rc = 0;
    pgcache_element_t *el;
    pgcache_t *pgcache;
    kfs_superblock_t *kfs_sb;


    TRACE("start");
    if( sb->sb_magic != KFS_MAGIC){
        TRACE_ERR("Magic does not match");
        return( -1);
    }

    if( kfs_active() != 0 ){
        TRACE_ERR("Superblock is not active");
        rc = -1;
    }

    pgcache = (pgcache_t *) sb->sb_page_cache;
    el = ( pgcache_element_t *) sb->sb_superblock_page; 
 
    sb->sb_flags = sb->sb_flags &~ KFS_IS_MOUNTED;

    kfs_sb = ( kfs_superblock_t *) el->pe_mem_ptr;
    sb_to_kfssb( kfs_sb, sb);

    
    TRACE("ok2");

    pgcache_destroy( pgcache);
    close( sb->sb_bdev);
    memset( &__sb, 0, sizeof( sb_t ));
    TRACE("end rc=%d", rc);
    return(rc);
}

    

void sb_to_kfssb( kfs_superblock_t *ksb, sb_t *sb){
    kfs_extent_t *kex;
    extent_t *ex;
    time_t now;

   
    now = time( NULL);

    ksb->sb_m_time = sb->sb_m_time = now; 
    ksb->sb_a_time = sb->sb_a_time = now; 

    ksb->sb_root_super_inode = sb->sb_root_super_inode;
    ksb->sb_flags = sb->sb_flags;

    ksb->sb_si_table.capacity = sb->sb_si_table.capacity;
    ksb->sb_si_table.in_use = sb->sb_si_table.in_use;
    kex = &ksb->sb_si_table.table_extent;
    ex = &sb->sb_si_table.table_extent;
    ex_2_kfsex( kex, ex);
    kex = &ksb->sb_si_table.bitmap_extent;
    ex = &sb->sb_si_table.bitmap_extent;
    ex_2_kfsex( kex, ex);

    ksb->sb_slot_table.capacity = sb->sb_slot_table.capacity;
    ksb->sb_slot_table.in_use = sb->sb_slot_table.in_use;
    kex = &ksb->sb_slot_table.table_extent;
    ex = &sb->sb_slot_table.table_extent;
    ex_2_kfsex( kex, ex);
    kex = &ksb->sb_slot_table.bitmap_extent;
    ex = &sb->sb_slot_table.bitmap_extent;
    ex_2_kfsex( kex, ex);


    ksb->sb_blockmap.capacity = sb->sb_blockmap.capacity;
    ksb->sb_blockmap.in_use = sb->sb_blockmap.in_use;
    kex = &ksb->sb_blockmap.table_extent;
    ex = &sb->sb_blockmap.table_extent;
    ex_2_kfsex( kex, ex);
    kex = &ksb->sb_blockmap.bitmap_extent;
    ex = &sb->sb_blockmap.bitmap_extent;
    ex_2_kfsex( kex, ex);
}


