AS := as
CC := gcc


CFLAGS := -Iinclude -fno-builtin -fno-stack-protector \
		  -fomit-frame-pointer \
		  -ffreestanding -nostdinc \
		  -fno-asynchronous-unwind-tables \
		  -fno-pic -fno-dwarf2-cfi-asm -Wall -g3

all: mbr/mbr.bin kernel/kernel.bin

tools/link/link: tools/link/link.go
	make -C tools/link

mbr/mbr.bin: tools/link/link
	make -C mbr
	dd if=$@ of=hda.img conv=notrunc

kernel/kernel.bin: tools/link/link
	make -C kernel

remote := ssh://arch@arch.rs/home/arch/projects/linux/ut
qemu:
	rsync -azvp arch@arch.rs:projects/linux/ut/kernel/kernel.bin /tmp/kernel.bin
	qemu-system-i386 -cpu 'SandyBridge' -monitor stdio -echr 0x14 \
		-hda $(remote)/hda.img \
		-device loader,file=/tmp/kernel.bin,addr=0x100000,force-raw=on \
		-device loader,file=disk.img,addr=0x800000,force-raw=on \
		-serial /dev/ttys001 \
		-m 64 -display curses -S -s


clean:
	make -C mbr clean
	make -C kernel clean

