CFLAGS = -Wall -fcommon -fno-builtin -I include -I .
KERNEL = src/*.c util/*.c sys/disk/disk.c sys/mem/mem.c

empty-iso:
	dd if=/dev/zero bs=4096 count=161 > sys/empty-fs.iso

fs-maker:
	gcc $(CFLAGS) fs-maker.c sys/disk/disk.c -o bin/fs-maker

# Run on a freshly formatted disk: make empty-iso fs-maker && ./bin/fs-maker
.PHONY: test
test:
	gcc $(CFLAGS) test/test1_host_to_fs.c $(KERNEL) -o bin/test1
	gcc $(CFLAGS) test/test2_fs_to_host.c $(KERNEL) -o bin/test2
	gcc $(CFLAGS) test/test3_persist.c $(KERNEL) -o bin/test3
	gcc $(CFLAGS) test/test4_paths.c $(KERNEL) -o bin/test4

clean:
	rm -f bin/*
