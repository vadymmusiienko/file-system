#include "vnode.h"

static mobj_ops_t vnode_mobj_ops = {
    .read_pframe = vnode_read_pframe,
    .write_pframe = vnode_write_pframe,
    .flush_pframe = vnode_flush_pframe,
};

long vnode_read_pframe(mobj_t *o, long pagenum, pframe_t **pfp) {
    vnode_t *vn = (vnode_t *) o->parent;
    return vn->vn_ops->read_pframe(vn, pagenum, pfp);
}

long vnode_write_pframe(mobj_t *o, pframe_t *pfp) {
    vnode_t *vn = (vnode_t *) o->parent;
    return vn->vn_ops->write_pframe(vn, pfp);
}

long vnode_flush_pframe(mobj_t *o, pframe_t *pfp) {
    vnode_t *vn = (vnode_t *) o->parent;
    return vn->vn_ops->flush_pframe(vn, pfp);
}
