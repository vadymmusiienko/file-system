#include "svfs.h"

#include "dirent.h"
#include "slab_allocator.h"
#include "sys/disk/disk.h"
#include "util/string.h"
#include <stdio.h>

// Disk layout constants (must match fs-maker.c)
#define SV_NO_BLOCK (-1) // "no block" / "no inode" sentinel
#define SV_FREE_NEXT (SV_NFBLKS_PER_NODE - 1) // Free list slot of next chain

static svfs_t svfs; // Global instance of system v file system
static slab_allocator_t svfs_node_allocator;   // Global allocator for nodes
static slab_allocator_t svfs_pframe_allocator; // Global allocator for pframes
static slab_allocator_t svfs_dirent_allocator; // Global allocator for readdir

fs_ops_t svfs_fsops = {.fs_init = svfs_init, .read_vnode = svfs_read_vnode};

// Same as vnode_mobj_ops in vnode.c
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

// HELPERS

/*
 * Write len bytes of data to disk block blkno
 */
static void svfs_write_block(long blkno, const void *data, long len) {
    char block[SV_BLOCK_SIZE] = {0};
    memcpy(block, data, len);
    write_disk_block(blkno, block);
}

/*
 * Save the superblock to disk
 */
static void svfs_write_super(svfs_t *fs) {
    svfs_write_block(SV_SUPER_BLOCK, &fs->svfs_super, sizeof(svfs_super_t));
}

/*
 * Save a node's inode to disk (called whenever the inode changes)
 */
static void svfs_write_inode(svfs_node_t *node) {
    svfs_write_block(node->inode.svi_number, &node->inode,
                     sizeof(svfs_inode_t));
    node->dirtied_inode = 0;
}

/*
 * Take a data block off the free list and zero it on disk
 */
static long svfs_alloc_block() {
    svfs_super_t *super = &svfs.svfs_super;
    spinlock_lock(&svfs.svfs_mutex);

    // Get a free block listed in the superblock
    long blk = SV_NO_BLOCK;
    for (int i = 0; i < SV_FREE_NEXT && blk == SV_NO_BLOCK; i++) {
        if (super->svs_free_blocks[i] != SV_NO_BLOCK) {
            blk = super->svs_free_blocks[i];
            super->svs_free_blocks[i] = SV_NO_BLOCK;
        }
    }

    // None left
    // Refill the superblock's list from the next chain block, then
    // hand out the chain block itself (its contents now live in the superblock)
    if (blk == SV_NO_BLOCK &&
        super->svs_free_blocks[SV_FREE_NEXT] != SV_NO_BLOCK) {
        blk = super->svs_free_blocks[SV_FREE_NEXT];
        char block[SV_BLOCK_SIZE];
        read_disk_block(blk, block);
        memcpy(super->svs_free_blocks, block, sizeof(super->svs_free_blocks));
    }

    if (blk != SV_NO_BLOCK) {
        super->svs_nfree--;
        svfs_write_super(&svfs);
    }
    spinlock_unlock(&svfs.svfs_mutex);

    if (blk != SV_NO_BLOCK) {
        svfs_write_block(blk, "", 0); // New blocks start out as zeros
    }
    return blk;
}

/*
 * Find the disk block that holds page pagenum of a file
 * If the page has no block yet, allocate one when alloc is set
 */
static long svfs_page_block(svfs_node_t *node, long pagenum, int alloc) {
    svfs_inode_t *inode = &node->inode;
    if (pagenum < 0 || pagenum >= (long)SV_MAX_FILE_BLOCKS) {
        return SV_NO_BLOCK;
    }

    // Direct blocks: the inode points at them
    if (pagenum < SV_NDIRECT_BLKS) {
        if (inode->svi_direct_blocks[pagenum] == SV_NO_BLOCK && alloc) {
            long blk = svfs_alloc_block();
            if (blk == SV_NO_BLOCK) {
                return SV_NO_BLOCK;
            }
            inode->svi_direct_blocks[pagenum] = blk;
            svfs_write_inode(node);
        }
        return inode->svi_direct_blocks[pagenum];
    }

    // Indirect block: a block full of block numbers for the remaining pages
    long ptrs[SV_NIDIRECT_BLKS]; // Exactly one block
    if (inode->svi_indirect_block == SV_NO_BLOCK) {
        if (!alloc) {
            return SV_NO_BLOCK;
        }
        long blk = svfs_alloc_block();
        if (blk == SV_NO_BLOCK) {
            return SV_NO_BLOCK;
        }
        for (long i = 0; i < (long)SV_NIDIRECT_BLKS; i++) {
            ptrs[i] = SV_NO_BLOCK;
        }
        write_disk_block(blk, (char *)ptrs);
        inode->svi_indirect_block = blk;
        svfs_write_inode(node);
    }

    read_disk_block(inode->svi_indirect_block, (char *)ptrs);
    long idx = pagenum - SV_NDIRECT_BLKS;
    if (ptrs[idx] == SV_NO_BLOCK && alloc) {
        long blk = svfs_alloc_block();
        if (blk == SV_NO_BLOCK) {
            return SV_NO_BLOCK;
        }
        ptrs[idx] = blk;
        write_disk_block(inode->svi_indirect_block, (char *)ptrs);
    }
    return ptrs[idx];
}

/*
 * Get the vnode of inode ino
 * Load it into cache (vfs list) if not there
 */
static vnode_t *svfs_get_vnode(long ino) {
    spinlock_lock(&vfs_root_fs.vnode_list_mutex);

    for (list_link_t *link = vfs_root_fs.vnode_list.head; link != NULL;
         link = link->next) {
        vnode_t *vn = (vnode_t *)link->parent;
        if (((svfs_node_t *)vn->parent)->inode.svi_number == ino) {
            spinlock_unlock(&vfs_root_fs.vnode_list_mutex);
            return vn; // Found
        }
    }

    // svfs_read_vnode needs parent and svi_number set first
    svfs_node_t *node = slab_obj_alloc(svfs_node_allocator);
    node->vnode.parent = node;
    node->inode.svi_number = ino;
    vfs_root_fs.fs_ops->read_vnode(&vfs_root_fs, &node->vnode);
    list_insert_back(&vfs_root_fs.vnode_list, &node->vnode.vn_link);

    spinlock_unlock(&vfs_root_fs.vnode_list_mutex);
    return &node->vnode;
}

/*
 * Append the entry name to the end of directory dir and save the
 * directory to disk
 */
static long svfs_add_dirent(vnode_t *dir, const char *name, long namelen,
                            long ino) {
    dirent_t ent = {0}; // Name stays '\0'-terminated
    ent.d_ino = ino;
    ent.d_off = dir->vn_len + sizeof(dirent_t);
    memcpy(ent.d_name, name, namelen);

    if (svfs_write(dir, dir->vn_len, &ent, sizeof(dirent_t)) !=
        (long)sizeof(dirent_t)) {
        return -1;
    }
    mobj_flush(&dir->vn_mobj);
    return 0;
}

/*
 * Helper for mknod and mkdir
 */
static long svfs_create(vnode_t *dir, const char *name, long namelen,
                        vn_mode_t mode, vnode_t **out) {
    if (dir->vn_mode != DIRECTORY || namelen <= 0 || namelen > NAME_LEN) {
        return -1;
    }

    // Names must be unique within a directory
    vnode_t *existing;
    if (svfs_lookup(dir, name, namelen, &existing) == 0) {
        return -1;
    }

    long ino = svfs_alloc_inode(&svfs);
    if (ino == SV_NO_BLOCK) {
        return -1;
    }

    // Write a fresh inode for the new file
    svfs_inode_t inode;
    inode.svi_un.svi_size = 0;
    inode.svi_number = ino;
    // Its entry in dir (and "." for a directory) refer to it
    inode.svi_count = mode == DIRECTORY ? 2 : 1;
    for (int i = 0; i < SV_NDIRECT_BLKS; i++) {
        inode.svi_direct_blocks[i] = SV_NO_BLOCK;
    }
    inode.svi_indirect_block = SV_NO_BLOCK;
    inode.svi_mode = mode;
    svfs_write_block(ino, &inode, sizeof(svfs_inode_t));

    vnode_t *vn = svfs_get_vnode(ino);

    // Directories start with "." (itself) and ".." (its parent)
    if (mode == DIRECTORY) {
        svfs_node_t *dir_node = (svfs_node_t *)dir->parent;
        if (svfs_add_dirent(vn, ".", 1, ino) != 0 ||
            svfs_add_dirent(vn, "..", 2, dir_node->inode.svi_number) != 0) {
            return -1;
        }
        dir_node->inode.svi_count++; // The new ".." refers to dir
        svfs_write_inode(dir_node);
    }

    if (svfs_add_dirent(dir, name, namelen, ino) != 0) {
        return -1;
    }

    *out = vn;
    return 0;
}

// ----------------------

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

    // Allocators for file pages and readdir results
    slab_allocator_init(&svfs_pframe_allocator, sizeof(pframe_t));
    slab_allocator_init(&svfs_dirent_allocator, sizeof(dirent_t));

    // Root directory (loaded and added to the vnode list)
    vfs_root_fs.fs_root = svfs_get_vnode(svfs.svfs_super.svs_root_inode);
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
 * Take an unused inode off the free inode list
 */
long svfs_alloc_inode(svfs_t *svfs) {
    spinlock_lock(&svfs->svfs_mutex);

    long ino = svfs->svfs_super.svs_free_inode;
    // Out of memory
    if (ino == SV_NO_BLOCK) {
        spinlock_unlock(&svfs->svfs_mutex);
        return -1;
    }

    // Read in free node from disk
    svfs_inode_t free_inode;
    char block[SV_BLOCK_SIZE];
    read_disk_block(ino, block);
    memcpy(&free_inode, block, sizeof(svfs_inode_t));

    // Update the free list
    svfs->svfs_super.svs_free_inode = free_inode.svi_un.svi_next_free;
    svfs_write_super(svfs);

    spinlock_unlock(&svfs->svfs_mutex);
    return ino;
}

/*
 * Find the directory entry called name in directory dir and store its inode
 * number in ent->inode.svi_number
 */
long svfs_find_dirent(svfs_node_t *dir, const char *name, long namelen,
                      svfs_node_t *ent) {
    if (namelen <= 0 || namelen > NAME_LEN) {
        return -1;
    }

    // A directory's data is an array of dirents
    dirent_t d;
    for (long pos = 0; pos < dir->vnode.vn_len; pos += sizeof(dirent_t)) {
        if (svfs_read(&dir->vnode, pos, &d, sizeof(dirent_t)) !=
            (long)sizeof(dirent_t)) {
            return -1;
        }

        // Same name check
        int same = d.d_name[namelen] == '\0';
        for (long i = 0; same && i < namelen; i++) {
            same = d.d_name[i] == name[i];
        }
        if (same) {
            ent->inode.svi_number = d.d_ino;
            return 0;
        }
    }

    return -1;
}

/*
 * Read up to len bytes from the file
 */
long svfs_read(vnode_t *vn, long pos, void *buf, long len) {
    if (pos < 0 || len < 0) {
        return -1;
    }

    // Don't read past the end of the file
    if (pos >= vn->vn_len) {
        return 0;
    }
    if (len > vn->vn_len - pos) {
        len = vn->vn_len - pos;
    }
    if (len == 0) {
        return 0;
    }

    // Make sure every page we need is in memory, then read
    for (long page = pos / FRAME_LEN; page <= (pos + len - 1) / FRAME_LEN;
         page++) {
        pframe_t *pf;
        mobj_get_pframe(&vn->vn_mobj, page, &pf);
        if (pf == NULL) {
            return -1;
        }
    }
    return mobj_read(&vn->vn_mobj, pos, len, buf);
}

/*
 * Write len bytes from buf into the file at position pos
 */
long svfs_write(vnode_t *vn, long pos, const void *buf, long len) {
    if (pos < 0 || len < 0 || pos + len > (long)SV_MAX_FILE_SIZE) {
        return -1;
    }
    if (len == 0) {
        return 0;
    }

    svfs_node_t *node = (svfs_node_t *)vn->parent;
    long end = pos + len;

    // Every page from the current end of the file (so writing past the end
    // leaves no holes) up to the last byte written needs a block in memory
    long first = (pos < vn->vn_len ? pos : vn->vn_len) / FRAME_LEN;
    for (long page = first; page <= (end - 1) / FRAME_LEN; page++) {
        if (svfs_page_block(node, page, 1) == SV_NO_BLOCK) {
            return -1;
        }
        pframe_t *pf;
        mobj_get_pframe(&vn->vn_mobj, page, &pf);
        if (pf == NULL) {
            return -1;
        }
    }
    long written = mobj_write(&vn->vn_mobj, pos, len, buf);

    // Save the new size
    if (end > vn->vn_len) {
        vn->vn_len = end;
        node->inode.svi_un.svi_size = end;
        svfs_write_inode(node);
    }
    return written;
}

/*
 * Creates a new entry for a file
 */
long svfs_mknod(vnode_t *dir, const char *name, long namelen, vnode_t **out) {
    return svfs_create(dir, name, namelen, REGULAR, out);
}

/*
 * Creates a new entry for a directory
 */
long svfs_mkdir(vnode_t *dir, const char *name, long namelen, vnode_t **out) {
    return svfs_create(dir, name, namelen, DIRECTORY, out);
}

/*
 * Search for a given entry within a directory
 */
long svfs_lookup(vnode_t *dir, const char *name, long namelen, vnode_t **out) {
    if (dir->vn_mode != DIRECTORY) {
        return -1;
    }

    // find_dirent only fills in ent.inode.svi_number
    svfs_node_t ent;
    if (svfs_find_dirent((svfs_node_t *)dir->parent, name, namelen, &ent) !=
        0) {
        return -1;
    }

    *out = svfs_get_vnode(ent.inode.svi_number);
    return 0;
}

/*
 * Retrieve the directory entry at byte position pos in the provided directory
 */
long svfs_readdir(vnode_t *vnode, long pos, dirent_t **d) {
    if (vnode->vn_mode != DIRECTORY || pos < 0 ||
        pos % (long)sizeof(dirent_t) != 0 || pos >= vnode->vn_len) {
        return -1;
    }

    dirent_t *ent = slab_obj_alloc(svfs_dirent_allocator);
    if (svfs_read(vnode, pos, ent, sizeof(dirent_t)) !=
        (long)sizeof(dirent_t)) {
        return -1;
    }

    *d = ent;
    return 0;
}

/*
 * Read page pagenum of a file from its disk block into a new pframe
 */
long svfs_read_pframe(vnode_t *vnode, long pagenum, pframe_t **pfp) {
    long blk = svfs_page_block((svfs_node_t *)vnode->parent, pagenum, 0);
    if (blk == SV_NO_BLOCK) {
        return -1;
    }

    // mobj_get_pframe links the new pframe into the mobj's list
    pframe_t *pf = slab_obj_alloc(svfs_pframe_allocator);
    pf->pf_pagenum = pagenum;
    pf->pf_addr = pf->bytes;
    read_disk_block(blk, pf->bytes);

    *pfp = pf;
    return 0;
}

/*
 * Write pframe to its disk block
 */
long svfs_write_pframe(vnode_t *vnode, pframe_t *pf) {
    long blk = svfs_page_block((svfs_node_t *)vnode->parent, pf->pf_pagenum, 0);
    if (blk == SV_NO_BLOCK) {
        return -1;
    }

    write_disk_block(blk, pf->bytes);
    return 0;
}
