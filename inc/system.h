#ifndef __SYSTEM_H
#define __SYSTEM_H
#include <stdarg.h>

typedef int size_t;

/* This defines what the stack looks like after an ISR was running */
typedef struct regs
{
    unsigned int gs, fs, es, ds;
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    unsigned int int_no, err_code;
    unsigned int eip, cs, eflags, useresp, ss;
}STR_REGS;

//debug flags
//#define _DEBUG_FLAG_BLK
#define _DEBUG_FLAG_FS

//end debug flags

//inline assembly utilities
#define sti() __asm__ ("sti"::)
#define cli() __asm__ ("cli"::)
#define nop() __asm__ ("nop"::)

#define outb(value,port) \
__asm__ ("outb %%al,%%dx"::"a" (value),"d" (port))


#define inb(port) ({ \
unsigned char _v; \
__asm__ volatile ("inb %%dx,%%al":"=a" (_v):"d" (port)); \
_v; \
})

#define outb_p(value,port) \
__asm__ ("outb %%al,%%dx\n" \
		"\tjmp 1f\n" \
		"1:\tjmp 1f\n" \
		"1:"::"a" (value),"d" (port))

#define inb_p(port) ({ \
unsigned char _v; \
__asm__ volatile ("inb %%dx,%%al\n" \
	"\tjmp 1f\n" \
	"1:\tjmp 1f\n" \
	"1:":"=a" (_v):"d" (port)); \
_v; \
})

//end inline assembly utilities
/* UTIL.C */
extern void *memcpy(void *dest, const void *src, size_t count);
extern void *memset(void *dest, char val, size_t count);
extern unsigned short *memsetw(unsigned short *dest, unsigned short val, size_t count);
extern size_t strlen(const char *str);
//extern unsigned char inportb (unsigned short _port);
//extern void outportb (unsigned short _port, unsigned char _data);
extern int vsprintf(char *buf, const char *fmt, va_list args);
extern void printk(const char *fmt, ...);
extern void panic(const char * s);

/* CONSOLE.C */
extern void init_video(void);
extern void puts(const char *text);
extern void putch(unsigned char c);
extern void putx(unsigned int d);
extern void cls();

/* GDT.C */
extern void gdt_set_gate(int num, unsigned long base, unsigned long limit, unsigned char access, unsigned char gran);
extern void gdt_install();

/* IDT.C */
extern void idt_set_gate(unsigned char num, unsigned long base, unsigned short sel, unsigned char flags);
extern void idt_install();

/* ISRS.C */
extern void isrs_install();

/* IRQ.C */
extern void irq_install_handler(int irq, void (*handler)(void));
extern void irq_uninstall_handler(int irq);
extern void irq_install();

/* TIMER.C */
extern void timer_wait(unsigned long ticks);
extern void timer_install();

/* KEYBOARD.C */
extern void keyboard_install();

#endif

