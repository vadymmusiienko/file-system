#ifndef __PFRAME_H__
#define __PFRAME_H__

#include "util/list.h"

#define FRAME_LEN 4096

typedef struct pframe {
    long pf_pagenum;
    void *pf_addr;
    list_link_t pf_link;

    char bytes[FRAME_LEN];
} pframe_t;

#endif // __PFRAME_H__
