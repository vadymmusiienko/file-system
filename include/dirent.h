#ifndef __DIRENT_H__
#define __DIRENT_H__

#define NAME_LEN 24

typedef struct dirent {
    long d_ino;                /* entry inode number */
    long d_off;                /* seek pointer of next entry */
    char d_name[NAME_LEN + 1]; /* filename */
} dirent_t;

#endif // __DIRENT_H__
