CC	= gcc
CFLAGS	= -Wall -Wextra -nostdlib -fno-builtin -nostartfiles -nodefaultlibs
LD	= ld
 
OBJFILES = hd.o blk_main.o buffer.o
 
all: blk.o
 
.s.o:
	as -o $@ $<
 
.c.o:
	$(CC) $(CFLAGS) -I../inc -o $@ -c $<
 
blk.o: $(OBJFILES)
	$(LD) -r -o $@ $^
 
 
clean:
	$(RM) *.o
