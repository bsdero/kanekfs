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

sb_t __sb = { 0};

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
        printf("KFS Version: %u\n", sb->sb_version);
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


int kfs_load_superblock( int fd, sb_t *sb, uint64_t rsi){
    char *p; 
    int rc; 
    kfs_superblock_t *kfs_sb;
    kfs_extent_t *kex;
    extent_t *ex;
    time_t now;
     
    p = pages_alloc( 1);
    if( p == NULL){
        TRACE_ERR("Superblock could not be loaded");
        return( -1);
    }

    rc = block_read( fd, p, 0);
    if( rc < 0){
        return( rc);
    }

    kfs_sb = (kfs_superblock_t *) p;

    if( kfs_sb->sb_magic != KFS_MAGIC){
        TRACE_ERR("Not a KFS file system. Abort.");
        return( -1);
    }

    memset( sb, 0, sizeof( sb_t));
    now = time( NULL);

    kfs_sb->sb_m_time = kfs_sb->sb_a_time = now; 

    sb->sb_root_super_inode = rsi;
    sb->sb_flags = kfs_sb->sb_flags;
    sb->sb_c_time = kfs_sb->sb_c_time;
    sb->sb_m_time = kfs_sb->sb_m_time;
    sb->sb_a_time = kfs_sb->sb_a_time;
    sb->sb_bdev = fd;
    
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


    sb->sb_flags = kfs_sb->sb_flags |= KFS_IS_MOUNTED;

    block_write( fd, p, 0);
    free( p); 

    return(0);
}


int kfs_open( kfs_config_t *config){
    int rc, fd; 
    sb_t sb;


    rc = kfs_verify( config->kfs_file, 0, 0);
    if( rc < 0){
        TRACE_ERR("Verification failed, abort");
        return( rc);
    }


    fd = open( config->kfs_file, O_RDWR );
    if( fd < 0){
        TRACE_ERR( "ERROR: Could not open file '%s'\n", config->kfs_file);
        return( -1);
    }

    rc = kfs_load_superblock( fd, &sb, config->root_super_inode);
    if( rc < 0){
        TRACE_ERR("Could not load superblock, abort");
        return( rc);
    }

    sb.sb_magic = KFS_MAGIC;
    __sb = sb;


    return( 0);
}

int kfs_active(){
    sb_t *sb = &__sb;

    if( sb->sb_magic != KFS_MAGIC){
        return( -1);
    }

    if( ( sb->sb_flags & KFS_IS_MOUNTED) == 0){
        return( -1);
    }

    return(0);
}

void kfs_superblock_display(){
    sb_t *sb = &__sb;
    char buff[240];
    extent_t *e;


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


}

int kfs_superblock_update(){
    char *p; 
    int rc; 
    kfs_superblock_t *ksb;
    kfs_extent_t *kex;
    extent_t *ex;
    time_t now;
    sb_t *sb = &__sb;

    if( kfs_active() != 0 ){
        TRACE_ERR("Superblock is not active");
        return( -1);
    }
   
    p = pages_alloc( 1);
    if( p == NULL){
        TRACE_ERR("Superblock could not be updated");
        return( -1);
    }


    rc = block_read( sb->sb_bdev, p, 0);
    if( rc < 0){
        return( rc);
    }

    ksb = (kfs_superblock_t *) p;

    if( ksb->sb_magic != KFS_MAGIC){
        TRACE_ERR("Not a KFS file system. Abort.");
        return( -1);
    }

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


    block_write( sb->sb_bdev, p, 0);
    free( p); 
    return(0);
}

int kfs_superblock_close(){
    sb_t *sb = &__sb;
    int rc;

    if( kfs_active() != 0 ){
        TRACE_ERR("Superblock is not active");
        return( -1);
    }
 
    sb->sb_flags = sb->sb_flags &~ KFS_IS_MOUNTED;
    rc = kfs_superblock_update();

    close( sb->sb_bdev);
    memset( &__sb, 0, sizeof( sb_t ));

    return(rc);
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



int kfs_slot_reserve( uint64_t *slot_id ){
    char *p;
    unsigned char *bitmap;
    uint64_t slotmap_blocks_num, slotmap_block, total_slots;
    uint64_t slot_block, slots_per_block, slot_offset; 
    kfs_extent_header_t *ex_header;
    int rc;
    sb_t *sb = &__sb;
    kfs_slot_t *kslot;


    if( kfs_active() != 0 ){
        TRACE_ERR("Superblock is not active");
        return( -1);
    }
    
    /* check if we have enough room for an extra slot */
    if( sb->sb_slot_table.in_use > sb->sb_slot_table.capacity ){
        TRACE_ERR("No more slots available");
        return( -1);
    }

    slotmap_block = sb->sb_slot_table.bitmap_extent.ex_block_addr;
    slotmap_blocks_num = sb->sb_slot_table.bitmap_extent.ex_block_size;

    /* alloc blocks */
    p = pages_alloc( slotmap_blocks_num);
    if( p == NULL){
        TRACE_ERR("Memory for SlotMap could not be reserved");
        return( -1);
    }

    /* read extent */
    rc = extent_read( sb->sb_bdev, p, slotmap_block, slotmap_blocks_num);
    if( rc == 0){
        TRACE_ERR("Could not read all, rc=%d", rc);
        return( -1);
    }

    ex_header = (kfs_extent_header_t *) p;

    /* verify this is a slot map block */
    if( ex_header->eh_magic != KFS_SLOTS_BITMAP_MAGIC){
        TRACE_ERR("Not a slot map block");
        return( -1);
    }

    /* double check the map has enough room for an extra slot, but
     * now we check in the map extent */
    if( ex_header->eh_entries_in_use > ex_header->eh_entries_capacity ){
        TRACE_ERR("No more slots available");
        return( -1);
    }

    total_slots = sb->sb_slot_table.capacity; 
/*    ex_header->eh_entries_in_use = 0;
    ex_header->eh_entries_capacity = bc->out_slots_num;
    ex_header->eh_flags = KFS_ENTRIES_ROOT|KFS_ENTRIES_LEAF;*/
 
    bitmap = (unsigned char *) p + sizeof( kfs_extent_header_t);
    rc = bm_find( bitmap, 
                  total_slots, 
                  0,
                  total_slots,
                  1,
                  slot_id);


    if( rc != 0){
        TRACE_ERR("A slot could not be found in the map");
        return( -1);
    }

    /* mark the slot as busy */
    rc = bm_set_bit( bitmap, total_slots, *slot_id, 1);

    /* update superblock data */
    sb->sb_slot_table.in_use++;

    /* update slot map header */
    ex_header->eh_entries_in_use = sb->sb_slot_table.in_use;

    /* write extent */
    rc = extent_write( sb->sb_bdev, p, slotmap_block, slotmap_blocks_num);
    if( rc == 0){
        TRACE_ERR("Could not write, rc=%d", rc);
        return( -1);
    }

    free( p);


    /* next, update slot and mark it as used */
    slots_per_block = (KFS_BLOCKSIZE - sizeof( kfs_extent_header_t)) / 
                      sizeof( kfs_slot_t);

    
    slot_offset = 0;
    slot_block = ( *slot_id * slots_per_block);
    if( slot_block == 0){
        slot_offset = sizeof( kfs_extent_t);
    }

    if( slot_block > sb->sb_si_table.table_extent.ex_block_size){
        TRACE_ERR( "slot block is outside the slots table. ");
        TRACE_ERR( "slot=[%lu, 0x%lx], slot_block=[%lu, 0x%lx]", 
                   *slot_id, *slot_id,
                    slot_block, slot_block);
        return( -1);
    }

    slot_block += sb->sb_si_table.table_extent.ex_block_addr;

    p = pages_alloc( 1);
    if( p == NULL){
        TRACE_ERR("Memory for SlotMap could not be reserved");
        return( -1);
    }


    rc = block_read( sb->sb_bdev, p, slot_block);
    if( rc == 0){
        TRACE_ERR("Could not read page, rc=%d", rc);
        return( -1);
    }

    kslot = ( kfs_slot_t *) p + (( *slot_id % KFS_BLOCKSIZE) + slot_offset);
    if( kslot->slot_id == *slot_id){
        TRACE_ERR("Could not find slot. ");
        TRACE_ERR("slot_id=[%lu, 0x%lx], got: [%lu, 0x%lx]",
                  *slot_id, *slot_id,
                   kslot->slot_id, kslot->slot_id);

        return( -1);
    }


    kslot->slot_flags |= SLOT_IN_USE;

    rc = block_write( sb->sb_bdev, p, slot_block);
    if( rc == 0){
        TRACE_ERR("Could not write page, rc=%d", rc);
        return( -1);
    }

    /* and update the slot index */
    rc = block_read( sb->sb_bdev, p, 
                     sb->sb_si_table.table_extent.ex_block_addr);
    if( rc == 0){
        TRACE_ERR("Could not read page, rc=%d", rc);
        return( -1);
    }

    ex_header = (kfs_extent_header_t *) p;

    /* verify this is a slot table block */
    if( ex_header->eh_magic != KFS_SLOTS_TABLE_MAGIC){
        TRACE_ERR("Not a slot table block");
        return( -1);
    }

    ex_header->eh_entries_in_use = sb->sb_slot_table.in_use;
    /* and update the slot index */
    rc = block_write( sb->sb_bdev, p, 
                      sb->sb_si_table.table_extent.ex_block_addr);
    if( rc == 0){
        TRACE_ERR("Could not write page, rc=%d", rc);
        return( -1);
    }

    free(p);
    return(0);
}

int kfs_slot_remove(uint64_t slot_id){
    char *p;
    unsigned char *bitmap;
    uint64_t slotmap_blocks_num, slotmap_block, total_slots;
    uint64_t slot_block, slots_per_block, slot_offset; 
    kfs_extent_header_t *ex_header;
    int rc;
    sb_t *sb = &__sb;
    kfs_slot_t *kslot;


    if( kfs_active() != 0 ){
        TRACE_ERR("Superblock is not active");
        return( -1);
    }
    
    /* check if we have enough room for an extra slot */
    if( slot_id > sb->sb_slot_table.capacity ){
        TRACE_ERR( "Wrong slot id beyond capacity. ");
        TRACE_ERR( "slot_id=[%lu, 0x%lx]",
                    slot_id, slot_id);
        return( -1);
    }

    slotmap_block = sb->sb_slot_table.bitmap_extent.ex_block_addr;
    slotmap_blocks_num = sb->sb_slot_table.bitmap_extent.ex_block_size;

    /* alloc blocks */
    p = pages_alloc( slotmap_blocks_num);
    if( p == NULL){
        TRACE_ERR("Memory for SlotMap could not be reserved");
        return( -1);
    }

    /* read extent */
    rc = extent_read( sb->sb_bdev, p, slotmap_block, slotmap_blocks_num);
    if( rc == 0){
        TRACE_ERR("Could not read all, rc=%d", rc);
        return( -1);
    }

    ex_header = (kfs_extent_header_t *) p;

    /* verify this is a slot map block */
    if( ex_header->eh_magic != KFS_SLOTS_BITMAP_MAGIC){
        TRACE_ERR("Not a slot map block");
        return( -1);
    }

    total_slots = sb->sb_slot_table.capacity; 
/*    ex_header->eh_entries_in_use = 0;
    ex_header->eh_entries_capacity = bc->out_slots_num;
    ex_header->eh_flags = KFS_ENTRIES_ROOT|KFS_ENTRIES_LEAF;*/
 
    bitmap = (unsigned char *) p + sizeof( kfs_extent_header_t);

    /* clean the slot  */
    rc = bm_set_bit( bitmap, total_slots, slot_id, 0);

    /* update superblock data */
    sb->sb_slot_table.in_use--;

    /* update slot map header */
    ex_header->eh_entries_in_use = sb->sb_slot_table.in_use;

    /* write extent */
    rc = extent_write( sb->sb_bdev, p, slotmap_block, slotmap_blocks_num);
    if( rc == 0){
        TRACE_ERR("Could not write, rc=%d", rc);
        return( -1);
    }

    free( p);


    /* next, update slot and mark it as used */
    slots_per_block = (KFS_BLOCKSIZE - sizeof( kfs_extent_header_t)) / 
                      sizeof( kfs_slot_t);

    
    slot_offset = 0;
    slot_block = ( slot_id * slots_per_block);
    if( slot_block == 0){
        slot_offset = sizeof( kfs_extent_t);
    }

    if( slot_block > sb->sb_si_table.table_extent.ex_block_size){
        TRACE_ERR( "slot block is outside the slots table. ");
        TRACE_ERR( "slot=[%lu, 0x%lx], slot_block=[%lu, 0x%lx]", 
                   slot_id, slot_id,
                   slot_block, slot_block);
        return( -1);
    }

    slot_block += sb->sb_si_table.table_extent.ex_block_addr;

    p = pages_alloc( 1);
    if( p == NULL){
        TRACE_ERR("Memory for SlotMap could not be reserved");
        return( -1);
    }


    rc = block_read( sb->sb_bdev, p, slot_block);
    if( rc == 0){
        TRACE_ERR("Could not read page, rc=%d", rc);
        return( -1);
    }

    kslot = ( kfs_slot_t *) p + (( slot_id % KFS_BLOCKSIZE) + slot_offset);
    if( kslot->slot_id == slot_id){
        TRACE_ERR("Could not find slot.");
        TRACE_ERR("slot_id=[%lu, 0x%lx], got: [%lu, 0x%lx]",
                   slot_id, slot_id,
                   kslot->slot_id, kslot->slot_id);

        return( -1);
    }


    kslot->slot_flags = 0;

    rc = block_write( sb->sb_bdev, p, slot_block);
    if( rc == 0){
        TRACE_ERR("Could not write page, rc=%d", rc);
        return( -1);
    }

    /* and update the slot index */
    rc = block_read( sb->sb_bdev, p, 
                     sb->sb_si_table.table_extent.ex_block_addr);
    if( rc == 0){
        TRACE_ERR("Could not read page, rc=%d", rc);
        return( -1);
    }

    ex_header = (kfs_extent_header_t *) p;

    /* verify this is a slot table block */
    if( ex_header->eh_magic != KFS_SLOTS_TABLE_MAGIC){
        TRACE_ERR("Not a slot table block");
        return( -1);
    }

    ex_header->eh_entries_in_use = sb->sb_slot_table.in_use;
    /* and update the slot index */
    rc = block_write( sb->sb_bdev, p, 
                      sb->sb_si_table.table_extent.ex_block_addr);
    if( rc == 0){
        TRACE_ERR("Could not write page, rc=%d", rc);
        return( -1);
    }

    free(p);
    return(0);

}


