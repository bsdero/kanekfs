#include <stdio.h>
#include <stdlib.h>
#include "kfs_disk.h"

int main(){
    printf("kfs_extent_header_t=%ld\n", sizeof( kfs_extent_header_t));
    printf("kfs_extent_t=%ld\n", sizeof( kfs_extent_t));
    printf("kfs_slot_t=%ld\n", sizeof( kfs_slot_t));
    printf("kfs_slot_data_t=%ld\n", sizeof( kfs_slot_data_t));
    printf("kfs_edge_t=%ld\n", sizeof( kfs_edge_t));
    printf("kfs_sinode_t=%ld\n", sizeof( kfs_sinode_t));
    printf("kfs_table_t=%ld\n", sizeof( kfs_table_t));
    printf("kfs_slot_table_t=%ld\n", sizeof( kfs_slot_table_t));
    printf("kfs_si_table_t=%ld\n", sizeof( kfs_si_table_t));
    printf("kfs_blockmap_t=%ld\n", sizeof( kfs_blockmap_t));
    printf("kfs_superblock_t=%ld\n", sizeof( kfs_superblock_t));

    printf("SlotPerBlock=%ld, mod=%ld\n", 
            KFS_BLOCKSIZE/sizeof( kfs_slot_t),
            KFS_BLOCKSIZE%sizeof( kfs_slot_t));
     printf("SinodePerBlock=%ld, mod=%ld\n", 
            KFS_BLOCKSIZE/sizeof( kfs_sinode_t),
            KFS_BLOCKSIZE%sizeof( kfs_sinode_t));
    return(0);
}

