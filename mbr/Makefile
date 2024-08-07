CC := gcc
CFLAGS := -fno-builtin -fno-stack-protector \
		  -fomit-frame-pointer \
		  -ffreestanding -nostdinc \
		  -fno-asynchronous-unwind-tables \
		  -fno-pic -fno-dwarf2-cfi-asm -Wall -g3

mbr.bin: mbr.elf
	../tools/link/link mbr.json

mbr.o: mbr.c
	$(CC) $(CFLAGS) -c -Os -m16 -o $@ $^

head.o: head.s
	$(AS) -32 -o $@ $^

jump.o: jump.c
	$(CC) $(CFLAGS) -c -m32 -o $@ $^


mbr.elf: mbr.o head.o jump.o
	ld -m elf_i386 -T mbr.lds -o $@ $^


clean:
	@rm *.o *.elf *.bin
