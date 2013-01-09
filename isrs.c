#include <system.h>

/* This is a simple string array. It contains the message that
*  corresponds to each and every exception. We get the correct
*  message by accessing like:
*  exception_message[interrupt_number] */
char *exception_messages[] =
{
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",

    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",

    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",

    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

/* All of our Exception handling Interrupt Service Routines will
*  point to this function. This will tell us what exception has
*  happened! Right now, we simply halt the system by hitting an
*  endless loop. All ISRs disable interrupts while they are being
*  serviced as a 'locking' mechanism to prevent an IRQ from
*  happening and messing up kernel data structures */
void fault_handler(struct regs *r)
{
    if (r->int_no < 32)
    {
        printk(exception_messages[r->int_no]);
        printk(" Exception. System Halted!\n");
        for (;;);
    }
}

__asm__ (
//0: Divide By Zero Exception
    "isr0:\t\n"
    "cli\t\n"
    "pushl $0\t\n"
    "pushl $0\t\n"
    "jmp isr_common_stub\t\n"
//1: Debug Exception
    "isr1:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $1\n\t"
    "jmp isr_common_stub\n\t"
//2: Non Maskable Interrupt Exception
    "isr2:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $2\n\t"
    "jmp isr_common_stub\n\t"
//3: Int 3 Exception
    "isr3:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $3\n\t"
    "jmp isr_common_stub\n\t"
//4: INTO Exception
    "isr4:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $4\n\t"
    "jmp isr_common_stub\n\t"
//5: Out of Bounds Exception
    "isr5:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $5\n\t"
    "jmp isr_common_stub\n\t"
//6: Invalid Opcode Exception
    "isr6:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $6\n\t"
    "jmp isr_common_stub\n\t"
//7: Coprocessor Not Available Exception
    "isr7:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $7\n\t"
    "jmp isr_common_stub\n\t"
//8: Double Fault Exception (With Error Code!)
    "isr8:\n\t"
    "cli\n\t"
    "pushl $8\n\t"
    "jmp isr_common_stub\n\t"
//9: Coprocessor Segment Overrun Exception
    "isr9:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $9\n\t"
    "jmp isr_common_stub\n\t"
//10: Bad TSS Exception (With Error Code!)
    "isr10:\n\t"
    "cli\n\t"
    "pushl $10\n\t"
    "jmp isr_common_stub\n\t"
//11: Segment Not Present Exception (With Error Code!)
    "isr11:\n\t"
    "cli\n\t"
    "pushl $11\n\t"
    "jmp isr_common_stub\n\t"
//12: Stack Fault Exception (With Error Code!)
    "isr12:\n\t"
    "cli\n\t"
    "pushl $12\n\t"
    "jmp isr_common_stub\n\t"
//13: General Protection Fault Exception (With Error Code!)
    "isr13:\n\t"
    "cli\n\t"
    "pushl $13\n\t"
    "jmp isr_common_stub\n\t"
//14: Page Fault Exception (With Error Code!)
    "isr14:\n\t"
    "cli\n\t"
    "pushl $14\n\t"
    "jmp isr_common_stub\n\t"
//15: Reserved Exception
    "isr15:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $15\n\t"
    "jmp isr_common_stub\n\t"
//16: Floating Point Exception
    "isr16:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $16\n\t"
    "jmp isr_common_stub\n\t"
//17: Alignment Check Exception
    "isr17:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $17\n\t"
    "jmp isr_common_stub\n\t"
//18: Machine Check Exception
    "isr18:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $18\n\t"
    "jmp isr_common_stub\n\t"
//19: Reserved
    "isr19:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $19\n\t"
    "jmp isr_common_stub\n\t"
//20: Reserved
    "isr20:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $20\n\t"
    "jmp isr_common_stub\n\t"
//21: Reserved
    "isr21:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $21\n\t"
    "jmp isr_common_stub\n\t"
//22: Reserved
    "isr22:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $22\n\t"
    "jmp isr_common_stub\n\t"
//23: Reserved
    "isr23:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $23\n\t"
    "jmp isr_common_stub\n\t"
//24: Reserved
    "isr24:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $24\n\t"
    "jmp isr_common_stub\n\t"
//25: Reserved
    "isr25:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $25\n\t"
    "jmp isr_common_stub\n\t"
//26: Reserved
    "isr26:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $26\n\t"
    "jmp isr_common_stub\n\t"
//27: Reserved
    "isr27:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $27\n\t"
    "jmp isr_common_stub\n\t"
//28: Reserved
    "isr28:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $28\n\t"
    "jmp isr_common_stub\n\t"
//29: Reserved
    "isr29:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $29\n\t"
    "jmp isr_common_stub\n\t"
//30: Reserved
    "isr30:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $30\n\t"
    "jmp isr_common_stub\n\t"
//31: Reserved
    "isr31:\n\t"
    "cli\n\t"
    "pushl $0\n\t"
    "pushl $31\n\t"
    "jmp isr_common_stub\n\t"
//This is our common ISR stub. It saves the processor state, sets
//up for kernel mode segments, calls the C-level fault handler,
//and finally restores the stack frame.
    "isr_common_stub:\t\n"
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
    "mov %esp, %eax\t\n"      //this line: for param struct regs *r
    "pushl %eax\t\n"          //this line: for param struct regs *r
    "call fault_handler\t\n"
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

extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();
/* This is a very repetitive function... it's not hard, it's
*  just annoying. As you can see, we set the first 32 entries
*  in the IDT to the first 32 ISRs. We can't use a for loop
*  for this, because there is no way to get the function names
*  that correspond to that given entry. We set the access
*  flags to 0x8E. This means that the entry is present, is
*  running in ring 0 (kernel level), and has the lower 5 bits
*  set to the required '14', which is represented by 'E' in
*  hex. */
void isrs_install()
{
//  putx((unsigned)isr0);
    idt_set_gate(0, (unsigned)isr0, 0x10, 0x8E);
    idt_set_gate(1, (unsigned)isr1, 0x10, 0x8E);
    idt_set_gate(2, (unsigned)isr2, 0x10, 0x8E);
    idt_set_gate(3, (unsigned)isr3, 0x10, 0x8E);
    idt_set_gate(4, (unsigned)isr4, 0x10, 0x8E);
    idt_set_gate(5, (unsigned)isr5, 0x10, 0x8E);
    idt_set_gate(6, (unsigned)isr6, 0x10, 0x8E);
    idt_set_gate(7, (unsigned)isr7, 0x10, 0x8E);

    idt_set_gate(8, (unsigned)isr8, 0x10, 0x8E);
    idt_set_gate(9, (unsigned)isr9, 0x10, 0x8E);
    idt_set_gate(10, (unsigned)isr10, 0x10, 0x8E);
    idt_set_gate(11, (unsigned)isr11, 0x10, 0x8E);
    idt_set_gate(12, (unsigned)isr12, 0x10, 0x8E);
    idt_set_gate(13, (unsigned)isr13, 0x10, 0x8E);
    idt_set_gate(14, (unsigned)isr14, 0x10, 0x8E);
    idt_set_gate(15, (unsigned)isr15, 0x10, 0x8E);

    idt_set_gate(16, (unsigned)isr16, 0x10, 0x8E);
    idt_set_gate(17, (unsigned)isr17, 0x10, 0x8E);
    idt_set_gate(18, (unsigned)isr18, 0x10, 0x8E);
    idt_set_gate(19, (unsigned)isr19, 0x10, 0x8E);
    idt_set_gate(20, (unsigned)isr20, 0x10, 0x8E);
    idt_set_gate(21, (unsigned)isr21, 0x10, 0x8E);
    idt_set_gate(22, (unsigned)isr22, 0x10, 0x8E);
    idt_set_gate(23, (unsigned)isr23, 0x10, 0x8E);

    idt_set_gate(24, (unsigned)isr24, 0x10, 0x8E);
    idt_set_gate(25, (unsigned)isr25, 0x10, 0x8E);
    idt_set_gate(26, (unsigned)isr26, 0x10, 0x8E);
    idt_set_gate(27, (unsigned)isr27, 0x10, 0x8E);
    idt_set_gate(28, (unsigned)isr28, 0x10, 0x8E);
    idt_set_gate(29, (unsigned)isr29, 0x10, 0x8E);
    idt_set_gate(30, (unsigned)isr30, 0x10, 0x8E);
    idt_set_gate(31, (unsigned)isr31, 0x10, 0x8E);
}

