#include "svfs.h"

#include "dirent.h"
#include "slab_allocator.h"
#include "sys/disk/disk.h"
#include "util/string.h"
#include <stdio.h>

static svfs_t svfs; // Global instance of system v file system
static slab_allocator_t svfs_node_allocator; // Global allocator for nodes

fs_ops_t svfs_fsops = {.fs_init = svfs_init, .read_vnode = svfs_read_vnode};

// Same as vnode_mobj_ops in vnode.c (which is static, so we can't use it):
// the mobj asks the vnode, which asks svfs_read_pframe/svfs_write_pframe
static mobj_ops_t svfs_mobj_ops = {.read_pframe = vnode_read_pframe,
                                   .write_pframe = vnode_write_pframe,
                                   .flush_pframe = vnode_flush_pframe};

static vnode_ops_t svfs_dir_vops = {.read = NULL,
                                    .write = NULL,
                                    .mknod = svfs_mknod,
                                    .mkdir = svfs_mkdir,
                                    .lookup = svfs_lookup,
                                    .readdir = svfs_readdir,
                                    .read_pframe = svfs_read_pframe,
                                    .write_pframe = svfs_write_pframe,
                                    .flush_pframe = NULL};

static vnode_ops_t svfs_file_vops = {.read = svfs_read,
                                     .write = svfs_write,
                                     .mknod = NULL,
                                     .mkdir = NULL,
                                     .lookup = NULL,
                                     .readdir = NULL,
                                     .read_pframe = svfs_read_pframe,
                                     .write_pframe = svfs_write_pframe,
                                     .flush_pframe = NULL};

/*
 * Initialize system v file system
 * Assumes mem_init() and slab_init() ran before
 */
void svfs_init() {
    // Load the superblock and make sure the disk holds a SVFS
    char block[SV_BLOCK_SIZE];
    read_disk_block(SV_SUPER_BLOCK, block);
    memcpy(&svfs.svfs_super, block, sizeof(svfs_super_t));
    if (svfs_check_super(&svfs.svfs_super) != 0) {
        printf("Super block check failed");
        return;
    }

    // svfs_t
    spinlock_init(&svfs.svfs_mutex);
    svfs.svfs_fs = &vfs_root_fs;

    // fs_t
    vfs_root_fs.fs_ops = &svfs_fsops;
    slab_allocator_init(&svfs_node_allocator, sizeof(svfs_node_t));
    vfs_root_fs.vnode_allocator = &svfs_node_allocator;
    list_init(&vfs_root_fs.vnode_list);
    spinlock_init(&vfs_root_fs.vnode_list_mutex);

    // Root vnode
    // svfs_read_vnode reads the inode number from svi_number
    svfs_node_t *root = slab_obj_alloc(svfs_node_allocator);
    root->inode.svi_number = svfs.svfs_super.svs_root_inode;
    root->vnode.parent = root; // From vnode to node
    vfs_root_fs.fs_ops->read_vnode(&vfs_root_fs, &root->vnode);

    // Set as fs root and add to the list
    vfs_root_fs.fs_root = &root->vnode;
    list_insert_back(&vfs_root_fs.vnode_list, &root->vnode.vn_link);
}

/*
 * Initialize a svfs_node/vnode after reading a file from disk
 * vn->parent and node->inode.svi_number must be set
 */
void svfs_read_vnode(fs_t *fs, vnode_t *vn) {
    // The caller points vn->parent at the svfs_node that holds this vnode
    svfs_node_t *node = (svfs_node_t *)vn->parent;

    // svfs_node_t
    // Load from disk
    long ino = node->inode.svi_number;
    char block[SV_BLOCK_SIZE];
    read_disk_block(ino, block);
    memcpy(&node->inode, block, sizeof(svfs_inode_t));
    node->inode.svi_number = ino; // Safety
    node->dirtied_inode = 0;

    // vnode_t
    vn->vn_mode = (vn_mode_t)node->inode.svi_mode;
    vn->vn_ops = vn->vn_mode == DIRECTORY ? &svfs_dir_vops : &svfs_file_vops;
    vn->vn_fs = fs;
    vn->vn_len = node->inode.svi_un.svi_size;
    list_link_init(&vn->vn_link, vn);

    // mobj_t
    mobj_create(&vn->vn_mobj, &svfs_mobj_ops);
    vn->vn_mobj.parent = vn;
}

/*
 * Check the super block. Return 0 on success, else -1
 *
 */
// TODO: Hardcoded constants for now
long svfs_check_super(svfs_super_t *super) {
    if (super->svs_magic != SV_MAGIC_NO) {
        return -1;
    }

    // Free inode list head is either empty or an inode block
    if (super->svs_free_inode != -1 &&
        (super->svs_free_inode < 1 || super->svs_free_inode > 32)) {
        return -1;
    }

    // A full disk (0 free) is still valid
    if (super->svs_nfree < 0 || super->svs_nfree > 128) {
        return -1;
    }

    // Every free list slot is either empty or a data block
    for (int i = 0; i < SV_NFBLKS_PER_NODE; i++) {
        long blk = super->svs_free_blocks[i];
        if (blk != -1 && (blk < 33 || blk > 160)) {
            return -1;
        }
    }

    // Always points to the first inode (second overall block)
    if (super->svs_root_inode != 1) {
        return -1;
    }

    // Always has exactly 32 inodes
    if (super->svs_num_inodes != 32) {
        return -1;
    }

    // Version is at least 1
    if (super->svs_version < 1) {
        return -1;
    }

    return 0;
}

/*
 * On file creation, this is called to attach an inode to the newly created
 * file. Choose an unused inode from the file system. Returns 0 on succes,
 * -1 on failure.
 *
 * TODO: implement me!
 *
 * Hints:
 *   - what is the next available inode?
 *   - where do we find the next available inode after we allocate this inode?
 */
long svfs_alloc_inode(svfs_t *svfs) { return -1; }

/*
 * Find a directory entry from a svfs_node with the input name/namelen and
 * pass the associated svfs_node/vnode to the ent pointer. Return 0 on success,
 * else -1.
 *
 * TODO: implement me!
 *
 * Hints:
 *   - how should we read a directory entry?
 */
long svfs_find_dirent(svfs_node_t *dir, const char *name, long namelen,
                      svfs_node_t *ent) {
    return -1;
}

/*
 * Read len bytes from the file associated with the vnode_t from position pos
 * and save the result into the buffer. Return 0 on success, -1 on failure.
 *
 * TODO: implement me!
 *
 * Hints:
 *   - where is the data?
 *   - how do we access data from a svfs_node/vnode?
 */
long svfs_read(vnode_t *vn, long pos, void *buf, long len) { return -1; }

/*
 * Write len bytes from the buffer into the file at position pos. If the write
 * requires more bytes than are currently available, svfs should allocate
 * another data block to the file. Returns 0 on success, else -1.
 *
 * TODO: implement me!
 *
 * Hints:
 *   - where do new data blocks come from?
 *   - where should we put the data relative to the vnode?
 */
long svfs_write(vnode_t *vn, long pos, const void *buf, long len) { return -1; }

/*
 * Creates a new entry for a file (e.g., a call to "touch") for a file that
 * does not yet exist. Creates the file in the provided directory with the
 * the provided name and passes the newly created vnode to the out parameter.
 * Returns 0 on success, -1 on failure.
 *
 * TODO: implement me!
 *
 * Hints:
 *   - don't forget to add the newly created vnode to any global tracking
 *     mechanisms!
 */
long svfs_mknod(vnode_t *dir, const char *name, long namelen, vnode_t **out) {
    return -1;
}

/*
 * Creates a new entry for a directory for a file that does not yet exist.
 * Creates the file in the provided directory with theprovided name and passes
 * the newly created vnode to the out parameter.  Returns 0 on success, -1 on
 * failure.
 *
 * TODO: implement me!
 *
 * Hints:
 *   - don't forget to add the newly created vnode to any global tracking
 *     mechanisms!
 */
long svfs_mkdir(vnode_t *dir, const char *name, long namelen, vnode_t **out) {
    return -1;
}

/*
 * Search for a given entry within a directory. If found, entry is passed back
 * via the out pointer. Returns 0 on success, -1 on failure.
 *
 * TODO: implement me!
 *
 * Hints:
 *   - recall that the data in a directory is a series of dirents (defined in
 *     dirent.h)
 *   - what happens if we can't find the directory?
 */
long svfs_lookup(vnode_t *dir, const char *name, long namelen, vnode_t **out) {
    return -1;
}

/*
 * Retrieve the directory entry at the location specified by pos in the
 * provided directory. Return 0 on success, -1 on failure.
 *
 * TODO: implement me!
 *
 * Hints:
 *   - index the data associated with the directory
 *   - what happens if pos is not aligned to the dirent?
 */
long svfs_readdir(vnode_t *vnode, long pos, dirent_t **d) { return -1; }

/*
 * Read the data associated with a file from disk. Return 0 on success, -1 on
 * error.
 *
 * TODO: implement me!
 *
 * Hints:
 *   - are there helper functions that can help us out?
 *   - how do we access these helpers?
 */
long svfs_read_pframe(vnode_t *vnode, long pagenum, pframe_t **pfp) {
    // NOTE: This is the function used in both vnode and mobj ops
    return -1;
}

/*
 * Write pframe to disk. Return 0 on success, -1 on error.
 *
 * TODO: implement me!
 */
long svfs_write_pframe(vnode_t *vnode, pframe_t *pf) { return -1; }
