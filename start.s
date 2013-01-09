.global start


.align 4
# Multiboot macros to make a few lines later more readable
.set MULTIBOOT_PAGE_ALIGN,    1<<0
.set MULTIBOOT_MEMORY_INFO,  1<<1
.set MULTIBOOT_AOUT_KLUDGE,  1<<16
.set MULTIBOOT_HEADER_MAGIC,    0x1BADB002
.set MULTIBOOT_HEADER_FLAGS,    MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO # | MULTIBOOT_AOUT_KLUDGE  if this one is uncommented, [GRUB] Error 7: Loading below 1MB is not supported holmes don't know why
.set CHECKSUM, -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

.long MULTIBOOT_HEADER_MAGIC
.long MULTIBOOT_HEADER_FLAGS
.long CHECKSUM


.align 0x1000
PageDirectory:
	.long PageTable + 0x3
	.rept 1022 .long 0x0
	.endr
	.long PageDirectory + 0x3 /*we set up a recursive page directory*/

/*we set up a identical page table for the first 4M*/
.align 0x1000
PageTable:
	.set i, 0
	.rept 1024
		.long (i << 12) | 3
		.set i, i+1
	.endr

.align 4
start:
	movl  $(_sys_stack), %esp    # set up the stack
	mov $(PageDirectory), %ecx /*this line fucks me*/
	mov %ecx, %cr3
	mov %cr0, %ecx
	or $0x80000000, %ecx
	mov %ecx, %cr0
	jmp pageenabled

pageenabled:
	/* Push the pointer to the Multiboot information structure. */
    pushl   %ebx
    /* Push the magic value. */
    pushl   %eax
    call  kmain

    cli
hang:
    hlt
    jmp   hang

.section .bss
# Here is the definition of our BSS section. Right now, we'll use
# it just to store the stack. Remember that a stack actually grows
# downwards, so we declare the size of the data before declaring
# the identifier '_sys_stack'

.set STACKSIZE, 8192                  #This reserves 8KBytes of memory here
.lcomm buff, STACKSIZE
_sys_stack:

