#include "mobj.h"

#include "pframe.h"
#include "util/string.h"

/*
 * Initialize a mobj by copying its ops
 */
void mobj_create(mobj_t *o, mobj_ops_t *ops) {
    // Copy over the ops
    o->mo_ops = *ops;

    // Initialize the list of pframes
    list_init(&o->mo_pframes);

    // Initialize the lock
    spinlock_init(&o->mo_mutex);
}

/*
 * Find a pframe in RAM (mobj) and set ptp to that pframe
 */
void mobj_find_pframe(mobj_t *o, long pagenum, pframe_t **pfp) {
    list_link_t *pframe_link = o->mo_pframes.head;

    // Traverse the list and find pagenum
    while (pframe_link != NULL) {
        pframe_t *pframe = (pframe_t *)pframe_link->parent;
        if (pframe->pf_pagenum == pagenum) {
            *pfp = pframe;
            return;
        }

        pframe_link = pframe_link->next;
    }

    // Not found
    *pfp = NULL;
}

/*
 * Helper function
 * Insert a pframe into the list so that pframes stay ordered by page number
 */
static void insert_pframe_sorted(list_t *list, pframe_t *pframe) {
    list_link_t *link = &pframe->pf_link;

    // Find the first pframe with a bigger page number
    list_link_t *next = list->head;
    while (next != NULL &&
           ((pframe_t *)next->parent)->pf_pagenum < pframe->pf_pagenum) {
        next = next->next;
    }

    if (next == NULL) {
        list_insert_back(list, link);
    } else if (next == list->head) {
        list_insert_front(list, link);
    } else {
        // Link in between next->prev and next
        link->prev = next->prev;
        link->next = next;
        next->prev->next = link;
        next->prev = link;
        list->size++;
    }
}

/*
 *  Get a pframe from RAM or Disk
 */
void mobj_get_pframe(mobj_t *o, long pagenum, pframe_t **pfp) {
    // Search RAM first
    mobj_find_pframe(o, pagenum, pfp);
    if (*pfp != NULL) {
        return;
    }

    // Fetch from disk
    if (o->mo_ops.read_pframe(o, pagenum, pfp) == -1) {
        // Error fetching from disk
        *pfp = NULL;
        return;
    }

    // Insert pframe into the list
    list_link_init(&(*pfp)->pf_link, *pfp);
    insert_pframe_sorted(&o->mo_pframes, *pfp);
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
long mobj_read(mobj_t *o, long pos, long len, char *buf) { return -1; }

/*
 * TODO: implement me!
 *
 * Hints:
 *   - pos describes where to write to in terms of bytes
 *   - len is the maximum number of bytes to write... when can we not read all
 *     of the way up to len?
 *   - save what we read to buf
 */
long mobj_write(mobj_t *o, long pos, long len, const char *buf) { return -1; }

/*
 * TODO: implement me!
 *
 * Hints:
 *   - make sure that disk is up to date in the places that we care about!
 */
void mobj_flush(mobj_t *o) {}
