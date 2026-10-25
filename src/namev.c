#include "vfs.h"
#include "vnode.h"

/*
 * Look up name in directory dir using the directory's own lookup op
 */
long namev_lookup(vnode_t *dir, const char *name, long namelen, vnode_t **out) {
    if (dir == NULL || name == NULL || namelen <= 0) {
        return -1;
    }

    // Only directories have a lookup op
    if (dir->vn_ops->lookup == NULL) {
        return -1;
    }
    return dir->vn_ops->lookup(dir, name, namelen, out);
}

/*
 * Resolve path to a vnode
 */
long namev_open(vnode_t *base, const char *path, vnode_t **res_vnode) {
    if (path == NULL) {
        return -1;
    }

    // Start dir
    vnode_t *cur =
        (path[0] == '/' || base == NULL) ? vfs_root_fs.fs_root : base;

    long i = 0;
    while (path[i] != '\0') {
        // Find the end of the next name
        while (path[i] == '/') {
            i++;
        }
        long start = i;
        while (path[i] != '/' && path[i] != '\0') {
            i++;
        }

        // Change directory
        if (i > start) {
            vnode_t *next;
            if (namev_lookup(cur, path + start, i - start, &next) != 0) {
                return -1;
            }
            cur = next;
        }
    }

    *res_vnode = cur;
    return 0;
}

/*
 * Get the entry called name inside directory base
 * Ignore "." and ".."
 */
long namev_get_child(vnode_t *base, const char *name, long namelen,
                     vnode_t **out) {
    int is_dot = namelen == 1 && name[0] == '.';
    int is_dotdot = namelen == 2 && name[0] == '.' && name[1] == '.';
    if (is_dot || is_dotdot) {
        return -1;
    }
    return namev_lookup(base, name, namelen, out);
}

/*
 * Get the parent directory
 */
long namev_get_parent(vnode_t *base, vnode_t **out) {
    return namev_lookup(base, "..", 2, out);
}

/*
 * Is a the same as b or somewhere inside directory b
 */
long namev_is_descendent(vnode_t *a, vnode_t *b) {
    if (a == NULL || b == NULL) {
        return -1;
    }

    // Compares pointers
    vnode_t *cur = a;
    while (cur != b) {
        vnode_t *parent;
        if (namev_get_parent(cur, &parent) != 0) {
            return -1;
        }
        if (parent == cur) {
            return 0; // NO, Reached the root without finding b
        }
        cur = parent;
    }
    return 1; // YES
}
