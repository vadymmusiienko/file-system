#include "sys/disk/disk.h"

#include <stdio.h>
#include <string.h>

void get_disk_name(char *to_ret) {
    FILE *cfg = fopen("config.cfg", "r");
    if (!cfg) {
        fprintf(stderr, "Could not read file config.cfg...\n");
        __builtin_trap();
    }

    // fgets(to_ret, MAX_DISK_NAME_LEN, cfg);

    if (strncpy(to_ret, "sys/", (size_t)5) == NULL) {
        fprintf(stderr, "Could not setup buffer for reading disk name...\n");
        __builtin_trap();
    }

    if (fscanf(cfg, "diskname : %s", &to_ret[4]) == EOF) {
        fprintf(stderr, "Error reading disk name...\n");
        __builtin_trap();
    }

    if (fclose(cfg) == EOF) {
        fprintf(stderr, "Error closing config...\n");
        __builtin_trap();
    }
}

int disk_init() {
    // testing get_disk_name
    char buf[MAX_DISK_NAME_LEN];
    memset(buf, 0, MAX_DISK_NAME_LEN);
    get_disk_name(buf);

    if (buf[0] == EOF) {
        return -1;
    }

    return 0;
}

void read_disk_block(long frame_id, char *buf) {
    char disk_name[MAX_DISK_NAME_LEN];
    get_disk_name(disk_name);

    FILE *disk = fopen(disk_name, "r");
    if (!disk) {
        fprintf(stderr, "Could not open disk...\n");
        __builtin_trap();
    }

    // move the cursor to the location in the disk
    fseek(disk, FRAME_LEN * frame_id, SEEK_SET);

    long n = FRAME_LEN;
    long bytes_read = 0;
    long result;
    while ((result = fread(buf, 1, n - bytes_read, disk)) > 0) {
        bytes_read += result;
    }

    if (fclose(disk) == EOF) {
        fprintf(stderr, "Error closing disk...\n");
        __builtin_trap();
    }
}

// void write_disk_block(long frame_id, const char *buf) {
//     char disk_name[MAX_DISK_NAME_LEN];
//     get_disk_name(disk_name);
//
//     FILE *disk = fopen(disk_name, "w+");
//     if (!disk) {
//         fprintf(stderr, "Could not open disk...\n");
//         __builtin_trap();
//     }
//
//     // move the cursor to the location in the disk
//     fseek(disk, FRAME_LEN * frame_id, SEEK_SET);
//
//     long n = strlen(buf) > FRAME_LEN ? FRAME_LEN : strlen(buf);
//     long bytes_written = 0;
//     long result;
//     while ((result = fwrite(buf, 1, n - bytes_written, disk)) > 0) {
//         bytes_written += result;
//     }
//
//     if (fclose(disk) == EOF) {
//         fprintf(stderr, "Error closing disk...\n");
//         __builtin_trap();
//     }
// }

void write_disk_block(long frame_id, const char *buf) {
    char disk_name[MAX_DISK_NAME_LEN];
    get_disk_name(disk_name);

    // NOTE: Fix truncating bug (w+ to r+)
    FILE *disk = fopen(disk_name, "r+");
    if (!disk) {
        fprintf(stderr, "Could not open disk...\n");
        __builtin_trap();
    }

    // move the cursor to the location in the disk
    fseek(disk, FRAME_LEN * frame_id, SEEK_SET);

    // NOTE: Fix len bug (always write the entire block)
    // long n = strlen(buf) > FRAME_LEN ? FRAME_LEN : strlen(buf);
    long n = FRAME_LEN;
    long bytes_written = 0;
    long result;
    while ((result = fwrite(buf, 1, n - bytes_written, disk)) > 0) {
        bytes_written += result;
    }

    if (fclose(disk) == EOF) {
        fprintf(stderr, "Error closing disk...\n");
        __builtin_trap();
    }
}
