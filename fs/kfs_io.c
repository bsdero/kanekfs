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
#include "kfs_io.h"



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
    char *s, buff[256];
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
    printf("    kfs_file=<%s>\n", conf->kfs_file);
    printf("    cache_page_len=%d\n", conf->cache_page_len);
    printf("    cache_ino_len=%d\n", conf->cache_ino_len);   
    printf("    cache_path_len=%d\n", conf->cache_path_len);
    printf("    max_clients=%d\n", conf->max_clients);
    printf("    cache_graph_len=%d\n", conf->cache_graph_len);
    printf("    threads_pool=%d\n", conf->threads_pool);
}


int kfs_verify( char *filename, int verbose, int extra_verification){
    char buff[256];
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

    si = ( kfs_sinode_t *) pex;
    i = 0;
    while( si->si_id < sb->sb_si_table.capacity-1){
       if( i >= KFS_BLOCKSIZE){
            TRACE_ERR("Unexpected, the super inode ID is beyond the page.");
            TRACE_ERR("last inode: [%lu-0x%lx]", si->si_id, si->si_id);
            return(-1);
        }
        si++;
        i += sizeof( kfs_sinode_t);
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

    slot = ( kfs_slot_t *) pex;
    i = 0;
    while( slot->slot_id < sb->sb_slot_table.capacity-1){
       if( i >= KFS_BLOCKSIZE){
            TRACE_ERR("Unexpected, the slot ID is beyond the page.");
            TRACE_ERR("last inode: [%u-0x%x]", 
                    slot->slot_id, slot->slot_id);
            return(-1);
        }
        slot++;
        i += sizeof( kfs_slot_t);
    }
   
    if( verbose){
        printf("    -All seems to be fine, last slot: [%u-0x%x]\n", 
                     slot->slot_id, slot->slot_id);
    }


    if( verbose){
        printf("\nChecking Map extents\n");
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


int kfs_open( kfs_config_t *conf){
    int rc; 


    rc = kfs_verify( conf->kfs_file, 0, 0);



    return(rc);
}


