minix: minix.go
	go build -o $@ -buildvcs=false

test: minix
	[[ -f ../../disk.img ]] || xz -d ../../disk.img.xz 
	./minix -p ../../disk.img ls /usr/src/linux-gdb.tgz
	./minix -p ../../disk.img cat /usr/src/linux-gdb.tgz | md5sum

