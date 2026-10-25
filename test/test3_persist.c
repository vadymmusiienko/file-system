// Test 3: a flushed file survives a "reboot"
// Run twice, as two separate programs:
//   test3_persist write   creates /persist.txt and flushes it to disk
//   test3_persist read    boots again and finds the same file

#include "slab_allocator.h"
#include "svfs.h"
#include <assert.h>
#include <stdio.h>

int main(int argc, char **argv) {
    assert(argc == 2);
    const char message[] = "still here after a reboot!";
    long len = sizeof(message) - 1;

    // Launch the file system
    mem_init();
    slab_init();
    svfs_init();
    vnode_t *root = vfs_root_fs.fs_root;
    vnode_t *file;

    if (argv[1][0] == 'w') {
        assert(root->vn_ops->mknod(root, "persist.txt", 11, &file) == 0);
        assert(file->vn_ops->write(file, 0, message, len) == len);
        mobj_flush(&file->vn_mobj);
        printf("test3: wrote /persist.txt\n");
        return 0;
    }

    // Nothing is shared with the "write" run except the disk image
    char data[64];
    assert(root->vn_ops->lookup(root, "persist.txt", 11, &file) == 0);
    assert(file->vn_ops->read(file, 0, data, sizeof(data)) == len);
    for (long i = 0; i < len; i++) {
        assert(data[i] == message[i]);
    }
    printf("test3: /persist.txt survived the reboot\n");
    return 0;
}
