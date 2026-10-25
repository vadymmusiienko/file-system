// Test 4: paths
// Builds /a/b.txt, then finds it by path with the namev functions

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

    vnode_t *a, *b, *found;
    assert(root->vn_ops->mkdir(root, "a", 1, &a) == 0);
    assert(a->vn_ops->mknod(a, "b.txt", 5, &b) == 0);

    assert(namev_open(NULL, "/a/b.txt", &found) == 0 && found == b);
    assert(namev_open(NULL, "/a/missing", &found) == -1);
    assert(namev_get_parent(a, &found) == 0 && found == root);

    printf("test4: paths work\n");
    return 0;
}
