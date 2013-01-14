CC	= gcc
CFLAGS	= -Wall -Wextra -nostdlib -fno-builtin -nostartfiles -nodefaultlibs
LD	= ld
 
OBJFILES = start.o kernel.o scrn.o gdt.o idt.o isrs.o irq.o timer.o util.o blk/blk.o fs/fs.o
 
all: kernel.img
 
.s.o:
	as -o $@ $<
 
.c.o:
	$(CC) $(CFLAGS) -I./inc -o $@ -c $<

blk/blk.o:
	(cd blk; make; cd ..)

fs/fs.o:
	(cd fs; make; cd ..)

kernel.bin: $(OBJFILES)
	$(LD) -T linker.ld -o $@ $^
 
kernel.img: kernel.bin
	dd if=/dev/zero of=pad.img bs=1 count=4056
	cat stage1 stage2 pad.img $< > $@

subsys:
	(cd blk; make; cd ..; cd fs; make; cd ..)

clean:
	$(RM) $(OBJFILES) kernel.bin *.img; cd blk; make clean; cd ..; cd fs; make clean; cd ..;
 
install:
	$(RM) $(OBJFILES) kernel.bin
