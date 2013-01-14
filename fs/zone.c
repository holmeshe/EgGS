#include <system.h>
#include <fs.h>

#define clear_block(addr) \
__asm__ __volatile__ ("cld\n\t" \
    "rep\n\t" \
    "stosl" \
    ::"a" (0),"c" (BLOCK_SIZE/4),"D" ((long) (addr)))

void delete_block(unsigned int dev, unsigned int block)
{
	invalidate_buff(dev, block);

	free_zone_bitmap(dev, block);
}

//get a empty slot from bitmap and set bitmap and return a new buffer accordingly
unsigned int create_block(unsigned int dev)
{
	struct buffer_head * bh;
	int block;

	block = add_zone_bitmap(dev);

	if (!(bh=get_buff(dev,block)))
		panic("new_block: cannot get block");
	if (bh->b_count != 1)
		panic("new block: count is != 1");
	clear_block(bh->b_data);
	bh->b_uptodate = 1;
	bh->b_dirt = 1;
	put_buff(bh);
	return block;
}

void delete_indirecte_block(int dev,int block)
{
	struct buffer_head * bh;
	unsigned short * p;
	int i;

	if (!block)
		return;
	if ((bh=read_buff(dev,block))) {
		p = (unsigned short *) bh->b_data;
		for (i=0;i<512;i++,p++)
			if (*p)
				delete_block(dev,*p);
		put_buff(bh);
	}
	delete_block(dev,block);
}

void delete_double_indirecte_block(int dev,int block)
{
	struct buffer_head * bh;
	unsigned short * p;
	int i;

	if (!block)
		return;
	if ((bh=read_buff(dev,block))) {
		p = (unsigned short *) bh->b_data;
		for (i=0;i<512;i++,p++)
			if (*p)
				delete_indirecte_block(dev,*p);
		put_buff(bh);
	}
	delete_block(dev,block);
}
