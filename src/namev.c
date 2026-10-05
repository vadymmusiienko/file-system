#include "vfs.h"
#include "vnode.h"

/*
 * TODO: implement me!
 */
long namev_lookup(vnode_t *dir, const char *name, long namelen,
                  vnode_t **out) {
    return -1;
}

/*
 * TODO: implement me!
 */
long namev_open(vnode_t *base, const char *path, vnode_t **res_vnode) {
    return -1;
}

/*
 * TODO: implement me!
 */
long namev_get_child(vnode_t *base, const char *name, long namelen,
                     vnode_t **out) {
    return -1;
}

/*
 * TODO: implement me!
 */
long namev_get_parent(vnode_t *base, vnode_t **out) {
    return -1;
}

/*
 * TODO: implement me!
 */
long namev_is_descendent(vnode_t *a, vnode_t *b) {
    return -1;
}
