#include <system.h>
#include <fs.h>


struct super_block super_block_list[NR_SUPER];
/* this is initialized in init/main.c */
int ROOT_DEV = 0x301;

#define set_bit(nr,addr) ({\
register int res ; \
__asm__ __volatile__("btsl %2,%3\n\tsetb %%al": \
"=a" (res):"0" (0),"r" (nr),"m" (*(addr))); \
res;})

#define clear_bit(nr,addr) ({\
register int res ; \
__asm__ __volatile__("btrl %2,%3\n\tsetnb %%al": \
"=a" (res):"0" (0),"r" (nr),"m" (*(addr))); \
res;})

#define find_first_zero(addr) ({ \
int __res; \
__asm__ __volatile__ ("cld\n" \
    "1:\tlodsl\n\t" \
    "notl %%eax\n\t" \
    "bsfl %%eax,%%edx\n\t" \
    "je 2f\n\t" \
    "addl %%edx,%%ecx\n\t" \
    "jmp 3f\n" \
    "2:\taddl $32,%%ecx\n\t" \
    "cmpl $8192,%%ecx\n\t" \
    "jl 1b\n" \
    "3:" \
    :"=c" (__res):"c" (0),"S" (addr)); \
__res;})


void lock_super(struct super_block * sb)
{
#ifdef _DEBUG_FLAG_LOCK
    printk("debug_fs:lock super block lock[%d]\n", sb->s_lock);
#endif
    cli();
    while (sb->s_lock)
//tpc                   sleep_on(&(sb->s_wait));
        sb->s_lock = 1;
    sti();
}

void unlock_super(struct super_block * sb)
{
#ifdef _DEBUG_FLAG_LOCK
    printk("debug_fs:unlock super block lock[%d]\n", sb->s_lock);
#endif
    cli();
    sb->s_lock = 0;
//tpc                     wake_up(&(sb->s_wait));
    sti();
}

void wait_on_super(struct super_block * sb)
{
#ifdef _DEBUG_FLAG_LOCK
    printk("debug_fs:wait on super block lock[%d]\n", sb->s_lock);
#endif
    cli();
    while (sb->s_lock)
//tpc                   sleep_on(&(sb->s_wait));
        sti();
}

struct super_block * find_super(unsigned int dev)
{
    struct super_block * s;

    if (!dev)
        return NULL;
    s = 0+super_block_list;
    while (s < NR_SUPER+super_block_list)
        if (s->s_dev == dev)
        {
            wait_on_super(s);
            if (s->s_dev == dev)
                return s;
            else
                s = 0+super_block_list;
        }
        else
        {
            s++;
        }
    return NULL;
}

void put_super(int dev)
{
    struct super_block * sb;

    int i;

    if (dev == ROOT_DEV)
    {
        printk("root diskette changed: prepare for armageddon\n\r");
        return;
    }
    if (!(sb = find_super(dev)))
        return;
    if (sb->s_imount)
    {
        printk("Mounted disk changed - tssk, tssk\n\r");
        return;
    }
    lock_super(sb);
    sb->s_dev = 0;
    for(i=0; i<I_MAP_SLOTS; i++)
        put_buff(sb->s_imap[i]);
    for(i=0; i<Z_MAP_SLOTS; i++)
        put_buff(sb->s_zmap[i]);
    unlock_super(sb);
    return;
}

struct super_block * read_super(unsigned int dev)
{
    struct super_block * s;
    struct buffer_head * bh;
    int i,block;

    if (!dev)
        return NULL;

    if ((s = find_super(dev)))
        return s;
    for (s = 0+super_block_list ;; s++)
    {
        if (s >= NR_SUPER+super_block_list)
            return NULL;
        if (!s->s_dev)
            break;
    }
    s->s_dev = dev;
    s->s_isup = NULL;
    s->s_imount = NULL;
    s->s_time = 0;
    s->s_rd_only = 0;
    s->s_dirt = 0;
    lock_super(s);
    if (!(bh = read_buff(dev,1)))
    {
        s->s_dev=0;
        unlock_super(s);
        return NULL;
    }
    //we copy content here
    *((struct d_super_block *) s) =
        *((struct d_super_block *) bh->b_data);
#ifdef _DEBUG_FLAG_FS
    printk("debug_fs:read super block ok dev[0x%x] ni[%d] nz[%d] imb[%d] zmb[%d]\n\r \
          ###fd[%d] ms[%d] magic[0x%x]\n", s->s_dev, s->s_ninodes, s->s_nzones,
           s->s_imap_blocks, s->s_zmap_blocks, s->s_firstdatazone, s->s_max_size, s->s_magic);
#endif
    put_buff(bh);
    if (s->s_magic != SUPER_MAGIC)
    {
        s->s_dev = 0;
        unlock_super(s);
        return NULL;
    }
    for (i=0; i<I_MAP_SLOTS; i++)
        s->s_imap[i] = NULL;
    for (i=0; i<Z_MAP_SLOTS; i++)
        s->s_zmap[i] = NULL;
    block=2;
    for (i=0 ; i < s->s_imap_blocks ; i++)
        //we occupy buff here
        if ((s->s_imap[i]=read_buff(dev,block)))
            block++;
        else
            break;
    for (i=0 ; i < s->s_zmap_blocks ; i++)
        //we occupy buff here
        if ((s->s_zmap[i]=read_buff(dev,block)))
            block++;
        else
            break;
    if (block != 2+s->s_imap_blocks+s->s_zmap_blocks)
    {
        for(i=0; i<I_MAP_SLOTS; i++)
            put_buff(s->s_imap[i]);
        for(i=0; i<Z_MAP_SLOTS; i++)
            put_buff(s->s_zmap[i]);
        s->s_dev=0;
        unlock_super(s);
        return NULL;
    }
    //the first bit of the bit map should be 1
    s->s_imap[0]->b_data[0] |= 1;
    s->s_zmap[0]->b_data[0] |= 1;
    unlock_super(s);
    return s;
}

int add_inode_bitmap(unsigned int dev)
{
    struct super_block * sb;
    struct buffer_head * bh;
    int i,j;
    int imap_pos = 0;

    if (!(sb = find_super(dev)))
        panic("trying to get add_inode_bitmap from nonexistant device");
    j = 8192;
    for (i=0 ; i<sb->s_imap_blocks ; i++)
        if ((bh=sb->s_imap[i]))
            if ((j=find_first_zero(bh->b_data))<8192)
                break;
    imap_pos = j+i*8192;
    if (!bh || j >= 8192 || imap_pos > sb->s_ninodes)
    {
        printk("can't find empty inode map entry");
        return 0;
    }
    if (set_bit(j,bh->b_data))
        panic("add_inode_bitmap: bit already set");
    bh->b_dirt = 1;

    return imap_pos;
}

void free_inode_bitmap(unsigned int dev, unsigned int inode_num)
{
    struct super_block * sb;
    struct buffer_head * bh;


    if (!(sb = find_super(dev)))
        panic("trying to free inode on nonexistent device");
    if (inode_num < 1 || inode_num > sb->s_ninodes)
        panic("trying to free inode 0 or nonexistant inode");
    if (!(bh=sb->s_imap[inode_num>>13]))      //inode_num%8192
        panic("nonexistent imap in superblock");
    if (clear_bit(inode_num&8191,bh->b_data)) //inode_num mod 8192
        printk("free_inode: bit already cleared.\n\r");
    bh->b_dirt = 1;
}

int add_zone_bitmap(unsigned int dev)
{
    struct buffer_head * bh;
    struct super_block * sb;
    int i,j;
    int block = 0;

    if (!(sb = find_super(dev)))
        panic("trying to get new block from nonexistant device");
    j = 8192;
    for (i=0 ; i<sb->s_zmap_blocks ; i++)
        if ((bh=sb->s_zmap[i]))
            if ((j=find_first_zero(bh->b_data))<8192)
                break;
    block = (j+i*8192-1) + sb->s_firstdatazone;
    if (!bh || j >= 8192 || block > sb->s_nzones)
    {
        printk("can't find empty zone map entry");
        return 0;
    }

    if (set_bit(j,bh->b_data))
        panic("new_block: bit already set");
    bh->b_dirt = 1;

    return block;

}

void free_zone_bitmap(unsigned int dev, unsigned int block)
{
    struct super_block * sb;
    struct buffer_head * bh;

    if (!(sb = find_super(dev)))
        panic("trying to free block on nonexistent device");
    if (block < sb->s_firstdatazone || block >= sb->s_nzones)
        panic("trying to free block not in datazone");

    block -= sb->s_firstdatazone - 1 ;
    if (!(bh=sb->s_zmap[block>>13]))      //block%8192
        panic("nonexistent zmap in superblock");
    if (clear_bit(block&8191,bh->b_data)) //block mod 8192
        printk("free_block: bit already cleared.\n\r");

    bh->b_dirt = 1;
}

#ifdef _DEBUG_FLAG_FS
void testSuper()
{
    read_super(0x301);
    int i1 = add_inode_bitmap(0x301);
    int i2 = add_inode_bitmap(0x301);
    int i3 = add_inode_bitmap(0x301);

    int b1 = add_zone_bitmap(0x301);
    int b2 = add_zone_bitmap(0x301);
    int b3 = add_zone_bitmap(0x301);

    sync_dev(0x301);

    free_inode_bitmap(0x301, i1);
    free_inode_bitmap(0x301, i2);
    free_inode_bitmap(0x301, i3);

    free_zone_bitmap(0x301, b1);
    free_zone_bitmap(0x301, b2);
    free_zone_bitmap(0x301, b3);

    sync_dev(0x301);
}
#endif

