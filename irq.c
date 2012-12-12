/* bkerndev - Bran's Kernel Development Tutorial
*  By:   Brandon F. (friesenb@gmail.com)
*  Desc: Interrupt Request management
*
*  Notes: No warranty expressed or implied. Use at own risk. */
#include <system.h>

__asm__ (
//32: IRQ0
"irq0:\t\n"
	"cli\t\n"
    "pushl $0\t\n"
    "pushl $32\t\n"
    "jmp irq_common_stub\t\n"
//33: IRQ1
"irq1:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $33\n\t"
    "jmp irq_common_stub\n\t"
//34: IRQ2
"irq2:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $34\n\t"
    "jmp irq_common_stub\n\t"
//35: IRQ3
"irq3:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $35\n\t"
    "jmp irq_common_stub\n\t"
//36: IRQ4
"irq4:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $36\n\t"
    "jmp irq_common_stub\n\t"
//37: IRQ5
"irq5:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $37\n\t"
    "jmp irq_common_stub\n\t"
//38: IRQ6
"irq6:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $38\n\t"
    "jmp irq_common_stub\n\t"
//39: IRQ7
"irq7:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $39\n\t"
    "jmp irq_common_stub\n\t"
//40: IRQ8
"irq8:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $40\n\t"
    "jmp irq_common_stub\n\t"
//41: IRQ9
"irq9:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $41\n\t"
    "jmp irq_common_stub\n\t"
//42: IRQ10
"irq10:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $42\n\t"
    "jmp irq_common_stub\n\t"
//43: IRQ11
"irq11:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $43\n\t"
    "jmp irq_common_stub\n\t"
//44: IRQ12
"irq12:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $44\n\t"
    "jmp irq_common_stub\n\t"
//45: IRQ13
"irq13:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $45\n\t"
    "jmp irq_common_stub\n\t"
//46: IRQ14
"irq14:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $46\n\t"
    "jmp irq_common_stub\n\t"
//47: IRQ15
"irq15:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $47\n\t"
    "jmp irq_common_stub\n\t"
//This is our common irq stub. It saves the processor state, sets
//up for kernel mode segments, calls the C-level fault handler,
//and finally restores the stack frame.
	"irq_common_stub:\t\n"
    "pusha\t\n"
    "pushl %ds\t\n"
    "pushl %es\t\n"
    "pushl %fs\t\n"
    "pushl %gs\t\n"
    "mov $0x08, %ax\t\n"
    "mov %ax, %ds\t\n"
    "mov %ax, %es\t\n"
    "mov %ax, %fs\t\n"
    "mov %ax, %gs\t\n"
    "mov %esp, %eax\t\n"
    "pushl %eax\t\n"
    "call irq_handler\t\n"
    "popl %eax\t\n"
    "popl %gs\t\n"
    "popl %fs\t\n"
    "popl %es\t\n"
    "popl %ds\t\n"
    "popa\t\n"
    "add $0x8, %esp\t\n"
    "sti\t\n"
    "iret\t\n"
);


/* These are own ISRs that point to our special IRQ handler
*  instead of the regular 'fault_handler' function */
extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

/* This array is actually an array of function pointers. We use
*  this to handle custom IRQ handlers for a given IRQ */
void *irq_routines[16] =
{
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

/* This installs a custom IRQ handler for the given IRQ */
void irq_install_handler(int irq, void (*handler)(struct regs *r))
{
    irq_routines[irq] = handler;
}

/* This clears the handler for a given IRQ */
void irq_uninstall_handler(int irq)
{
    irq_routines[irq] = 0;
}

/* Normally, IRQs 0 to 7 are mapped to entries 8 to 15. This
*  is a problem in protected mode, because IDT entry 8 is a
*  Double Fault! Without remapping, every time IRQ0 fires,
*  you get a Double Fault Exception, which is NOT actually
*  what's happening. We send commands to the Programmable
*  Interrupt Controller (PICs - also called the 8259's) in
*  order to make IRQ0 to 15 be remapped to IDT entries 32 to
*  47 */
void irq_remap(void)
{
    outportb(0x20, 0x11);
    outportb(0xA0, 0x11);
    outportb(0x21, 0x20);
    outportb(0xA1, 0x28);
    outportb(0x21, 0x04);
    outportb(0xA1, 0x02);
    outportb(0x21, 0x01);
    outportb(0xA1, 0x01);
    outportb(0x21, 0x0);
    outportb(0xA1, 0x0);
}

/* We first remap the interrupt controllers, and then we install
*  the appropriate ISRs to the correct entries in the IDT. This
*  is just like installing the exception handlers */
void irq_install()
{
    irq_remap();

    idt_set_gate(32, (unsigned)irq0, 0x10, 0x8E);
    idt_set_gate(33, (unsigned)irq1, 0x10, 0x8E);
    idt_set_gate(34, (unsigned)irq2, 0x10, 0x8E);
    idt_set_gate(35, (unsigned)irq3, 0x10, 0x8E);
    idt_set_gate(36, (unsigned)irq4, 0x10, 0x8E);
    idt_set_gate(37, (unsigned)irq5, 0x10, 0x8E);
    idt_set_gate(38, (unsigned)irq6, 0x10, 0x8E);
    idt_set_gate(39, (unsigned)irq7, 0x10, 0x8E);

    idt_set_gate(40, (unsigned)irq8, 0x10, 0x8E);
    idt_set_gate(41, (unsigned)irq9, 0x10, 0x8E);
    idt_set_gate(42, (unsigned)irq10, 0x10, 0x8E);
    idt_set_gate(43, (unsigned)irq11, 0x10, 0x8E);
    idt_set_gate(44, (unsigned)irq12, 0x10, 0x8E);
    idt_set_gate(45, (unsigned)irq13, 0x10, 0x8E);
    idt_set_gate(46, (unsigned)irq14, 0x10, 0x8E);
    idt_set_gate(47, (unsigned)irq15, 0x10, 0x8E);
}

/* Each of the IRQ ISRs point to this function, rather than
*  the 'fault_handler' in 'isrs.c'. The IRQ Controllers need
*  to be told when you are done servicing them, so you need
*  to send them an "End of Interrupt" command (0x20). There
*  are two 8259 chips: The first exists at 0x20, the second
*  exists at 0xA0. If the second controller (an IRQ from 8 to
*  15) gets an interrupt, you need to acknowledge the
*  interrupt at BOTH controllers, otherwise, you only send
*  an EOI command to the first controller. If you don't send
*  an EOI, you won't raise any more IRQs */
void irq_handler(struct regs *r)
{
    /* This is a blank function pointer */
    void (*handler)(struct regs *r);
    
//    putx(r->int_no);
//    puts("\n");

    /* Find out if we have a custom handler to run for this
    *  IRQ, and then finally, run it */
    handler = irq_routines[r->int_no - 32];

    if ((unsigned long)handler != 0)
    {
//    	putx(handler);
        handler(r);
    }

    /* If the IDT entry that was invoked was greater than 40
    *  (meaning IRQ8 - 15), then we need to send an EOI to
    *  the slave controller */
    if (r->int_no >= 40)
    {
        outportb(0xA0, 0x20);
    }

    /* In either case, we need to send an EOI to the master
    *  interrupt controller too */
    outportb(0x20, 0x20);
}
