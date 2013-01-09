#include <multiboot.h>
#include <system.h>
#include <fs.h>

void kmain(unsigned long magic, multiboot_info_t * mbi)
{
   if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
   {
      printk ("Invalid magic number: 0x%x\n", (unsigned) magic);
      return;
   }

   gdt_install();
   idt_install();
   
   init_video();
   isrs_install();
   irq_install();
   timer_install();
   init_blk(mbi);
   __asm__ __volatile__ ("sti");

   printk("EgGS:hello there!\n");

//   int i = 10 / 0;

   for (;;);
}

