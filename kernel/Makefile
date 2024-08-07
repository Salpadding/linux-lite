CFLAGS := -I../include -fno-builtin -fno-stack-protector \
		  -fomit-frame-pointer \
		  -ffreestanding -nostdinc \
		  -fno-asynchronous-unwind-tables \
		  -fno-pic -fno-dwarf2-cfi-asm -Wall -g3

objs = kernel.o head.o print.o string.o \
	   memory.o buffer.o 8259a.o sched.o trap.o \
	   sys_call.o do_sys_call.o errno.o fork.o fs.o \
	   super.o

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c -m32 -O0 -g3 $^

%.s: %.c
	$(CC) $(CFLAGS) -o $@ -S -m32 -O0 $^

%.o: %.s
	$(AS) -o $@ --32 $^

kernel.bin: $(objs)
	ld -o kernel.elf -m elf_i386 -T kernel.lds $^
	../tools/link/link kernel.json
	chmod a+r $@

clean:
	rm *.o *.bin *.elf

