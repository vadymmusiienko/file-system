#ifndef __VFS_H__
#define __VFS_H__

#include "util/list.h"
#include "util/spinlock.h"

struct fs;
struct slab_allocator;
struct vnode;

typedef struct fs_ops {
    void (*fs_init)();
    void (*read_vnode)(struct fs *fs, struct vnode *vnode);
} fs_ops_t;

typedef struct fs {
    struct vnode *fs_root; // the root vnode of the file system

    fs_ops_t *fs_ops;

    struct slab_allocator *vnode_allocator;
    list_t vnode_list;
    spinlock_t vnode_list_mutex;
} fs_t;

fs_t vfs_root_fs;

// definitions in namev.c
long namev_lookup(struct vnode *dir, const char *name, long namelen,
                  struct vnode **out);

long namev_open(struct vnode *base, const char *path,
                struct vnode **res_vnode);

long namev_get_child(struct vnode *base, const char *name, long namelen,
                     struct vnode **out);

long namev_get_parent(struct vnode *base, struct vnode **out);

long namev_is_descendent(struct vnode *a, struct vnode *b);

#endif // __VFS_H__
