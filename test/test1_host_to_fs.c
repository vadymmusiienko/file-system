// Test 1: copy a file from the host into the file system
// Reads this source file from the host and saves it as /hello.txt

#include "slab_allocator.h"
#include "svfs.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    // Launch the file system
    mem_init();
    slab_init();
    svfs_init();
    vnode_t *root = vfs_root_fs.fs_root;

    // Read a file from the host
    char data[8192];
    FILE *host = fopen("test/test1_host_to_fs.c", "rb");
    assert(host != NULL);
    long len = fread(data, 1, sizeof(data), host);
    fclose(host);

    // Write it into the file system and save it to disk
    vnode_t *file;
    assert(root->vn_ops->mknod(root, "hello.txt", 9, &file) == 0);
    assert(file->vn_ops->write(file, 0, data, len) == len);
    assert(file->vn_len == len);
    mobj_flush(&file->vn_mobj);

    printf("test1: copied %ld bytes from the host into /hello.txt\n", len);
    return 0;
}
