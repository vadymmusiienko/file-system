#ifndef __DISK_H__
#define __DISK_H__

#include "pframe.h"

#define MAX_DISK_NAME_LEN 256

int disk_init();
void read_disk_block(long frame_id, char *buf);
void write_disk_block(long frame_id, const char *buf);

#endif // __DISK_H__
