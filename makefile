CC	= gcc
CFLAGS	= -Wall -Wextra -nostdlib -fno-builtin -nostartfiles -nodefaultlibs
LD	= ld
 
OBJFILES = start.o kernel.o scrn.o gdt.o idt.o isrs.o irq.o timer.o util.o blk/blk.o
 
all: kernel.img
 
.s.o:
	as -o $@ $<
 
.c.o:
	$(CC) $(CFLAGS) -I./inc -o $@ -c $<

blk/blk.o:
	(cd blk; make)

kernel.bin: $(OBJFILES)
	$(LD) -T linker.ld -o $@ $^
 
kernel.img: kernel.bin
	dd if=/dev/zero of=pad.img bs=1 count=4056
	cat stage1 stage2 pad.img $< > $@

subsys:
	(cd blk; make)

clean:
	$(RM) $(OBJFILES) kernel.bin *.img; cd blk; make clean
 
install:
	$(RM) $(OBJFILES) kernel.bin
