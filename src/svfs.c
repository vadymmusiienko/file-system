#include "svfs.h"

#include "dirent.h"

fs_ops_t svfs_fsops = { .fs_init = svfs_init,
                        .read_vnode = svfs_read_vnode};

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
 * TODO: implement me!
 *
 * Hints:
 *   - what is the main file system reference? how should our svfs instance
 *     refer to this main system?
 *   - it is your job here to implement both the fields of the svfs_t struct
 *     and the fs_t struct!
 */
void svfs_init() {
}

/*
 * Initialize a svfs_node/vnode after reading a file from disk.
 *
 * TODO: implement me!
 *
 * Hints:
 *   - how do we get data from disk?
 *   - what should the svfs_node/vnode fields be for a given file?
 *   - think about the distinction between normal file and directory!
 */
void svfs_read_vnode(fs_t *fs, vnode_t *vn) {
}


/*
 * Check the super block. Return 0 on success, else -1
 *
 * TODO: implement me!
 */
long svfs_check_super(svfs_super_t *super) {
    return -1;
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
long svfs_alloc_inode(svfs_t *svfs) {
    return -1;
}

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
long svfs_read(vnode_t *vn, long pos, char *buf, long len) {
    return -1;
}

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
long svfs_write(vnode_t *vn, long pos, const char *buf, long len) {
    return -1;
}

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
long svfs_readdir(vnode_t *vnode, long pos, dirent_t **d) {
    return -1;
}

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
    return -1;
}

/*
 * Write pframe to disk. Return 0 on success, -1 on error.
 *
 * TODO: implement me!
 */
long svfs_write_pframe(vnode_t *vnode, pframe_t *pf) {
    return -1;
}
