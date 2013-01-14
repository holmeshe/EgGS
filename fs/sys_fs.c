#include <system.h>
#include <fs.h>

#define test_bit(bitnr,addr) ({ \
register int __res ; \
__asm__("bt %2,%3;setb %%al":"=a" (__res):"a" (0),"r" (bitnr),"m" (*(addr))); \
__res; })


void sys_setup(multiboot_info_t * mbi)
{
	int i,free;
	struct super_block * p;
	struct m_inode * mi;

	init_blk(mbi);

	if (32 != sizeof (struct d_inode))
		panic("bad i-node size");
//tpc	for(i=0;i<NR_FILE;i++)
//tpc		file_table[i].f_count=0;
	if (MAJOR(ROOT_DEV) == 2) {
		panic("don't support floppy");
	}
	for(p = &super_block_list[0] ; p < &super_block_list[NR_SUPER] ; p++) {
		p->s_dev = 0;
		p->s_lock = 0;
//tpc		p->s_wait = NULL;
	}
	if (!(p=read_super(ROOT_DEV)))
		panic("Unable to mount root");
	if (!(mi=read_inode(ROOT_DEV,ROOT_INO)))
		panic("Unable to read root i-node");
	mi->i_count += 3 ;	/* NOTE! it is logically used 4 times, not 1 */
	p->s_isup = p->s_imount = mi;
//tpc	current->pwd = mi;
//tpc	current->root = mi;
	free=0;
	i=p->s_nzones;
	while (-- i >= 0)
		if (!test_bit(i&8191,p->s_zmap[i>>13]->b_data))
			free++;
	printk("%d/%d free blocks\n\r",free,p->s_nzones);
	free=0;
	i=p->s_ninodes+1;
	while (-- i >= 0)
		if (!test_bit(i&8191,p->s_imap[i>>13]->b_data))
			free++;
	printk("%d/%d free inodes\n\r",free,p->s_ninodes);
}

