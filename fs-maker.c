#include "include/dirent.h"
#include "include/svfs.h"
#include "sys/disk/disk.h"
#include <stdio.h>
#include <string.h>

// TODO: Move these to svfs?
#define SV_NINODES 32
#define SV_NDATA_BLKS 128
#define SV_INODE_START (SV_SUPER_BLOCK + 1)         // block 1
#define SV_DATA_START (SV_INODE_START + SV_NINODES) // block 33
#define SV_DATA_END (SV_DATA_START + SV_NDATA_BLKS) // 161
#define SV_NO_BLOCK (-1)  // "no block" sentinel for block pointers
#define SV_MODE_FREE (-1) // svi_mode of an unused inode

// NOTE: Declared in sys/disk/disk.c
void get_disk_name(char *to_ret);

// Wrapper for write_disk_block that 0's unused bits
static void write_block(long blkno, const void *data, size_t len) {
    char block[SV_BLOCK_SIZE];
    memset(block, 0, sizeof(block));
    memcpy(block, data, len);
    write_disk_block(blkno, block);
}

int main() {
    // Get the disk file name
    char buf[MAX_DISK_NAME_LEN];
    memset(buf, 0, MAX_DISK_NAME_LEN);
    get_disk_name(buf);

    // SUPERBLOCK
    svfs_super_t superblock;
    memset(&superblock, 0, sizeof(superblock));
    superblock.svs_magic = SV_MAGIC_NO;
    superblock.svs_free_inode =
        SV_INODE_START + 1;                   // The first one is taken by "/"
    superblock.svs_nfree = SV_NDATA_BLKS - 1; // Root is not free
    superblock.svs_root_inode = SV_INODE_START;
    superblock.svs_num_inodes = SV_NINODES;
    superblock.svs_version = 1;

    // Build the free block list
    // The first 29 slots hold free blocks and the
    // last slot points to a block that holds the next 29 (and so on)
    long next = SV_DATA_START + 1; // The first data block is taken by "/"

    for (int i = 0; i < SV_NFBLKS_PER_NODE - 1; i++) {
        superblock.svs_free_blocks[i] = next++;
    }
    superblock.svs_free_blocks[SV_NFBLKS_PER_NODE - 1] =
        next; // First chain block

    // Write Chain blocks
    while (next < SV_DATA_END) {
        long chain_block =
            next++; // This block stores the next part of the list
        long node[SV_NFBLKS_PER_NODE];

        for (int i = 0; i < SV_NFBLKS_PER_NODE - 1; i++) {
            node[i] = next < SV_DATA_END ? next++ : SV_NO_BLOCK;
        }
        node[SV_NFBLKS_PER_NODE - 1] = next < SV_DATA_END ? next : SV_NO_BLOCK;

        // Write all of the chain blocks to disk
        write_block(chain_block, node, sizeof(node));
    }

    // Write superblock to disk
    write_block(SV_SUPER_BLOCK, &superblock, sizeof(superblock));

    // INODES

    // First inode refers to root "/"
    svfs_inode_t root_inode;
    memset(&root_inode, 0, sizeof(root_inode));
    root_inode.svi_un.svi_size = 2 * sizeof(dirent_t); // "." and ".."
    root_inode.svi_number = SV_INODE_START;
    root_inode.svi_count = 2; // Both "." and ".." refer to root
    for (int i = 0; i < SV_NDIRECT_BLKS; i++) {
        root_inode.svi_direct_blocks[i] = SV_NO_BLOCK;
    }
    root_inode.svi_direct_blocks[0] = SV_DATA_START;
    root_inode.svi_indirect_block = SV_NO_BLOCK;
    root_inode.svi_mode = DIRECTORY;
    write_block(SV_INODE_START, &root_inode, sizeof(root_inode));

    // Other inodes are free and point to the next one
    for (int i = SV_INODE_START + 1; i < SV_DATA_START - 1; i++) {
        svfs_inode_t inode;
        memset(&inode, 0, sizeof(inode));
        inode.svi_un.svi_next_free = i + 1;
        inode.svi_mode = SV_MODE_FREE;

        // Write all of the inodes (implicit list)
        write_block(i, &inode, sizeof(inode));
    }

    // Last inode points to nothing
    svfs_inode_t last_inode;
    memset(&last_inode, 0, sizeof(last_inode));
    last_inode.svi_un.svi_next_free = SV_NO_BLOCK;
    last_inode.svi_mode = SV_MODE_FREE;
    write_block(SV_DATA_START - 1, &last_inode, sizeof(last_inode));

    // ROOT DATABLOCK
    dirent_t root_data[2];
    memset(root_data, 0, sizeof(root_data));
    root_data[0].d_ino = SV_INODE_START;
    strcpy(root_data[0].d_name, ".");
    root_data[1].d_ino = SV_INODE_START;
    strcpy(root_data[1].d_name, "..");
    write_block(SV_DATA_START, root_data, sizeof(root_data));

    return 0;
}
