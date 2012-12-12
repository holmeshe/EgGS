#ifndef __PAGE_H
#define __PAGE_H

//virtual memory layout
#define KERNEL_OFFSET 0x00100000
#define KERNEL_HEAP_START 0x20100000
#define KERNEL_HEAP_END 0x30100000
#define PAGE_STACK_VADDR 0x3E100000

#define PAGE_TABLES_VADDR 0xFFC00000 //Recursive Page Directory->page tables
#define PAGE_DIR_VADDR 0xFFFFF000 //Recursive Page Directory's last entry->page directory itself
//virtual memory layout end


#define PAGE_MASK 0xFFFFF000
#define PAGE_FLAG_MASK 0xFFF
#define PAGE_TABLE_MASK 0x003FF000

#define PAGE_SIZE 0x1000

#define PAGE_PRESENT 0x1
#define PAGE_WRITE 0x2
#define PAGE_USER 0x4

#define VMM_PAGES_PER_TABLE 1024

#define page_val(phaddr, flags) \
	((phaddr & PAGE_MASK) | (flags & PAGE_FLAG_MASK))
#define table_idx(vaddr) ((vaddr & PAGE_TABLE_MASK) >> 12)
#define dir_idx(vaddr) (vaddr >> 22)

#define vmm_flush_tlb(vaddr) \
	__asm__ volatile ("invlpg (%0)" : : "a" (vaddr & PAGE_MASK))

#define assert_higher(val)  \
	((unsigned int)(val) > KERNEL_OFFSET)?(val):\
		(__typeof__((val)))((unsigned int)(val)+ KERNEL_OFFSET)

unsigned int page_get(unsigned int vaddr);
void page_set(unsigned int vaddr, unsigned int value/*physical addr + flag*/);

void set_pd(unsigned int phaddr);

unsigned int pop_free_physical_page();
void push_free_phsical_page(unsigned int phaddr);

void page_init(mboot_info_t *mboot);

#endif
