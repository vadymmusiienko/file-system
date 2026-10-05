empty-iso:
	dd if=/dev/zero bs=4096 count=161 > sys/empty-fs.iso

# TODO: add your compilation settings here!

clean:
	rm -f bin/*
