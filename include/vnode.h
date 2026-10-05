#ifndef __VNODE_H__
#define __VNODE_H__

#include "mobj.h"
#include "pframe.h"

struct dirent;
struct fs;
struct vnode;

typedef enum {
    REGULAR,
    DIRECTORY
} vn_mode_t;

typedef struct vnode_ops {
    /*
     * Read the data assocaited with a file
     */
    long (*read)(struct vnode *file, long pos, void *buf, long count);

    /*
     * Write the provided data to a file
     */
    long (*write)(struct vnode *file, long pos, const void *buf, long count);

    /*
     * Create a new regular file
     */
    long (*mknod)(struct vnode *dir, const char *name, long namelen,
                  struct vnode **ret);

    /*
     * Create a new directory
     */
    long (*mkdir)(struct vnode *dir, const char *name, long namelen,
                  struct vnode **ret);

    /*
     * Find a file with the same name/namelen in the current directory
     */
    long (*lookup)(struct vnode *file, const char *name, long namelen,
                   struct vnode **ret);

    /*
     * Read an entry from a directory
     */
    long (*readdir)(struct vnode *dir, long pos, struct dirent **ret);

    /*
     * Call to read_pframe from the perspective of the vnode.
     */
    long (*read_pframe)(struct vnode *vnode, long pagenum, pframe_t **pfp);

    /*
     * Call to write_pframe from the perspective of the vnode
     */
    long (*write_pframe)(struct vnode *vnode, pframe_t *pfp);

    /*
     * Call to write_pframe from the perspective of the vnode
     */
    long (*flush_pframe)(struct vnode *vnode, pframe_t *pfp);
} vnode_ops_t;

typedef struct vnode {
    vnode_ops_t *vn_ops;
    struct fs *vn_fs;

    mobj_t vn_mobj;

    long vn_len;
    vn_mode_t vn_mode;

    list_link_t vn_link;

    void *parent;
} vnode_t;

long vnode_read_pframe(mobj_t *o, long pagenum, pframe_t **pfp);
long vnode_write_pframe(mobj_t *o, pframe_t *pfp);
long vnode_flush_pframe(mobj_t *o, pframe_t *pfp);

#endif // __VNODE_H__
