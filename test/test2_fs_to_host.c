// Test 2: copy a file from the file system onto the host
// Reads /hello.txt (written by test 1) and saves it as bin/hello.txt
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

    // Read /hello.txt from the file system
    vnode_t *file;
    assert(root->vn_ops->lookup(root, "hello.txt", 9, &file) == 0);
    char data[8192];
    long len = file->vn_ops->read(file, 0, data, sizeof(data));
    assert(len == file->vn_len);

    // Write it onto the host
    FILE *host = fopen("bin/hello.txt", "wb");
    assert(host != NULL);
    assert(fwrite(data, 1, len, host) == (size_t)len);
    fclose(host);

    // It must match the original - check manually
    printf("test2: copied /hello.txt onto the host as bin/hello.txt\n");
    return 0;
}
