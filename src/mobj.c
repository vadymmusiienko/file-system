#include "mobj.h"

#include "pframe.h"
#include "util/string.h"

/*
 * TODO: implement me!
 *
 * Hints:
 *   - when we write the ops, we want to make sure that we copy the value
 *     of the parameters into the space that we've been provided
 */
void mobj_create(mobj_t *o, mobj_ops_t *ops) {
}

/*
 * TODO: implement me!
 *
 * Hints:
 *   - in this function, where do we search for frame?
 *   - if we don't find the frame, set the pfp to NULL
 */
void mobj_find_pframe(mobj_t *o, long pagenum, pframe_t **pfp) {
}

/*
 * TODO: implement me!
 *
 * Hints:
 *   - the frame might be in our list already...
 *   - we have access to a function to read the pframe from disk!
 *   - check how we get data from disk in sys/disk/disk.{h,c}...
 */
void mobj_get_pframe(mobj_t *o, long pagenum, pframe_t **pfp) {
}

/*
 * TODO: implement me!
 *
 * Hints:
 *   - pos describes where to read from in terms of bytes
 *   - len is the maximum number of bytes to read... when can we not read all
 *     of the way up to len?
 *   - save what we read to buf
 */
long mobj_read(mobj_t *o, long pos, long len, char *buf) {
    return -1;
}

/*
 * TODO: implement me!
 *
 * Hints:
 *   - pos describes where to write to in terms of bytes
 *   - len is the maximum number of bytes to write... when can we not read all
 *     of the way up to len?
 *   - save what we read to buf
 */
long mobj_write(mobj_t *o, long pos, long len, const char *buf) {
    return -1;
}

/*
 * TODO: implement me!
 *
 * Hints:
 *   - make sure that disk is up to date in the places that we care about!
 */
void mobj_flush(mobj_t *o) {
}
