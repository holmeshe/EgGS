CC	= gcc
CFLAGS	= -Wall -Wextra -nostdlib -fno-builtin -nostartfiles -nodefaultlibs
LD	= ld
 
OBJFILES = super.o zone.o inode.o sys_fs.o
 
all: fs.o
 
.s.o:
	as -o $@ $<
 
.c.o:
	$(CC) $(CFLAGS) -I../inc -o $@ -c $<
 
fs.o: $(OBJFILES)
	$(LD) -r -o $@ $^
 
 
clean:
	$(RM) *.o
