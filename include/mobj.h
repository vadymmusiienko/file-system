#ifndef __MOBJ_H__
#define __MOBJ_H__

#include "util/list.h"
#include "util/spinlock.h"

struct mobj;
struct pframe;

typedef struct mobj_ops {
    long (*read_pframe)(struct mobj *o, long pagenum, struct pframe **pfp);
    long (*write_pframe)(struct mobj *o, struct pframe *pfp);
    long (*flush_pframe)(struct mobj *o, struct pframe *pfp);
} mobj_ops_t;

typedef struct mobj {
    mobj_ops_t mo_ops;

    list_t mo_pframes;
    spinlock_t mo_mutex;

    void *parent;
} mobj_t;

/*
 * Create a new mobj with the specified ops
 */
void mobj_create(mobj_t *o, mobj_ops_t *ops);

/*
 * Get pframe from mobj list (if it exists)
 */
void mobj_find_pframe(mobj_t *o, long pagenum, struct pframe **pfp);

/*
 * Wrapper for read_pframe from disk
 */
void mobj_get_pframe(mobj_t *o, long pagenum, struct pframe **pfp);

/*
 * Read data from pframe at specified location
 */
long mobj_read(mobj_t *o, long pos, long len, char *buf);

/*
 * Write data to pframe at specified location
 */
long mobj_write(mobj_t *o, long pos, long len, const char *buf);

/*
 * Flush changes to mobj in memory to disk
 */
void mobj_flush(mobj_t *o);

#endif // __MOBJ_H__
