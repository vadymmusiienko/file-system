#ifndef __SVFS_H__
#define __SVFS_H__

#include "vfs.h"
#include "vnode.h"

#include "util/list.h"
#include "util/spinlock.h"

#define SV_SUPER_BLOCK 0      // idx of the super block
#define SV_NFBLKS_PER_NODE 30 // artificially low to ensure everything fits
#define SV_BLOCK_SIZE 4096
#define SV_NDIRECT_BLKS 28
// NOTE: Had to fix these
#define SV_DIRENTS_PER_BLOCK (SV_BLOCK_SIZE / sizeof(dirent_t))
#define SV_NIDIRECT_BLKS (SV_BLOCK_SIZE / sizeof(long))
#define SV_MAX_FILE_BLOCKS (SV_NDIRECT_BLKS + SV_NIDIRECT_BLKS)
#define SV_MAX_FILE_SIZE (SV_MAX_FILE_BLOCKS * SV_BLOCK_SIZE)
#define SV_MAGIC_NO 134

typedef struct svfs_super {
    long svs_magic;      // tells a file system to mount that this is a SVFS
    long svs_free_inode; // pointer to the first free inode
    long svs_nfree;      // current number of free data blocks

    long svs_free_blocks[SV_NFBLKS_PER_NODE]; // free block list

    long svs_root_inode; // pointer to root inode
    long svs_num_inodes; // number of inodes in the system

    long svs_version; // version number for consistency
} svfs_super_t;

typedef struct svfs_inode {
    union {
        long svi_next_free;
        long svi_size;
    } svi_un; // either a pointer to the next free inode or the file size

    long svi_number; // inode number
    long svi_count;  // number of nodes that refer to this inode

    long svi_direct_blocks[SV_NDIRECT_BLKS]; // pointer to direct blocks
    long svi_indirect_block;                 // pointer to indirect block
} svfs_inode_t;

typedef struct svfs_node {
    vnode_t vnode;
    svfs_inode_t inode;
    long dirtied_inode;
} svfs_node_t;

typedef struct svfs {
    svfs_super_t svfs_super;
    spinlock_t svfs_mutex;
    fs_t *svfs_fs;
} svfs_t;

void svfs_init();

long svfs_check_super(svfs_super_t *super);

/**********
 * fs ops *
 **********/

long svfs_alloc_inode(svfs_t *svfs);

long svfs_find_dirent(svfs_node_t *dir, const char *name, long namelen,
                      svfs_node_t *ent);

/*************
 * vnode ops *
 *************/

void svfs_read_vnode(fs_t *fs, vnode_t *vn);

long svfs_read(vnode_t *vnode, long pos, void *buf, long len);

long svfs_write(vnode_t *vnode, long pos, const void *buf, long len);

long svfs_mknod(vnode_t *dir, const char *name, long namelen, vnode_t **out);

long svfs_mkdir(vnode_t *dir, const char *name, long namelen, vnode_t **out);

long svfs_lookup(vnode_t *dir, const char *name, long namelen, vnode_t **out);

long svfs_readdir(vnode_t *vnode, long pos, struct dirent **d);

long svfs_read_pframe(vnode_t *vnode, long pagenum, pframe_t **pfp);

long svfs_write_pframe(vnode_t *vnode, pframe_t *pf);

#endif // __SVFS_H__
