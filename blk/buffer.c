#include <system.h>
#include <fs.h>

extern int end;

struct buffer_head * start_buffer = (struct buffer_head *) &end;
struct buffer_head * hash_table[NR_HASH];
static struct buffer_head * free_list;
//tpc           static struct task_struct * buffer_wait = NULL;
int NR_BUFFERS = 0;

inline void wait_on_buff(struct buffer_head * bh)
{
    cli();
#ifdef _DEBUG_FLAG
    printk("wait on buff:%d\n", bh->b_lock);
#endif
    while (bh->b_lock)
//tpc                   sleep_on(&bh->b_wait);
    sti();
}

inline void lock_buff(struct buffer_head * bh)
{
    cli();
#ifdef _DEBUG_FLAG_BLK
    printk("debug_blk:lock buff:%d\n", bh->b_lock);
#endif
//tpc    while (bh->b_lock)
//tpc       sleep_on(&bh->b_wait);
    bh->b_lock=1;
    sti();
}

inline void unlock_buff(struct buffer_head * bh)
{
    if (!bh->b_lock)
        printk("free buffer being unlocked\n\r");
    bh->b_lock = 0;
//tpc   wake_up(&bh->b_wait);
}



void sync_dev(int dev)
{
    int i;
    struct buffer_head * bh;

    bh = start_buffer;
    for (i=0 ; i<NR_BUFFERS ; i++,bh++)
    {
        if (bh->b_dev != dev)
            continue;
        wait_on_buff(bh);
        if (bh->b_dev == dev && bh->b_dirt)
            ll_rw_block(WRITE,bh);
    }
}

inline void invalidate_buffers(int dev)
{
    int i;
    struct buffer_head * bh;

    bh = start_buffer;
    for (i=0 ; i<NR_BUFFERS ; i++,bh++)
    {
        if (bh->b_dev != dev)
            continue;
        wait_on_buff(bh);
        if (bh->b_dev == dev)
            bh->b_uptodate = bh->b_dirt = 0;
    }
}

#define _hashfn(dev,block) (((unsigned)(dev^block))%NR_HASH)
#define hash(dev,block) hash_table[_hashfn(dev,block)]

static inline void remove_from_queues(struct buffer_head * bh)
{
    /* remove from hash-queue */
    if (bh->b_next)
        bh->b_next->b_prev = bh->b_prev;
    if (bh->b_prev)
        bh->b_prev->b_next = bh->b_next;
    if (hash(bh->b_dev,bh->b_blocknr) == bh)
        hash(bh->b_dev,bh->b_blocknr) = bh->b_next;
    /* remove from free list */
    if (!(bh->b_prev_free) || !(bh->b_next_free))
        panic("Free block list corrupted");
    bh->b_prev_free->b_next_free = bh->b_next_free;
    bh->b_next_free->b_prev_free = bh->b_prev_free;
    if (free_list == bh)
        free_list = bh->b_next_free;
}

static inline void insert_into_queues(struct buffer_head * bh)
{
    /* put at end of free list */
    bh->b_next_free = free_list;
    bh->b_prev_free = free_list->b_prev_free;
    free_list->b_prev_free->b_next_free = bh;
    free_list->b_prev_free = bh;
    /* put the buffer in new hash-queue if it has a device */
    bh->b_prev = NULL;
    bh->b_next = NULL;
    if (!bh->b_dev)
        return;
    bh->b_next = hash(bh->b_dev,bh->b_blocknr);
    hash(bh->b_dev,bh->b_blocknr) = bh;
    bh->b_next->b_prev = bh;
}

static struct buffer_head * find_buffer_from_hash(unsigned int dev, unsigned int block)
{
    struct buffer_head * tmp;

    for (tmp = hash(dev,block) ; tmp != NULL ; tmp = tmp->b_next)
        if (tmp->b_dev==dev && tmp->b_blocknr==block)
        {
#ifdef _DEBUG_FLAG_BLK
            printk("debug_blk:find buff in hash table\n\r");
#endif
            return tmp;
        }
    return NULL;
}

struct buffer_head * query_buff(unsigned int dev, unsigned int block)
{
    struct buffer_head * bh;

    for (;;)
    {
        if (!(bh=find_buffer_from_hash(dev,block)))
            return NULL;
        bh->b_count++;
        wait_on_buff(bh);
        if (bh->b_dev == dev && bh->b_blocknr == block)
        {
#ifdef _DEBUG_FLAG_BLK
            printk("debug_blk:hit buff!\n\r");
#endif
            return bh;
        }
        bh->b_count--;
    }
}


/*
 * Ok, this is getblk, and it isn't very clear, again to hinder
 * race-conditions. Most of the code is seldom used, (ie repeating),
 * so it should be much more efficient than it looks.
 *
 * The algoritm is changed: hopefully better, and an elusive bug removed.
 */
#define BADNESS(bh) (((bh)->b_dirt<<1)+(bh)->b_lock)
struct buffer_head * get_buff(unsigned int dev, unsigned int block)
{
    struct buffer_head * tmp, * bh;

repeat:
    if ((bh = query_buff(dev,block)))
        return bh;
    tmp = free_list;
    do
    {
        if (tmp->b_count)
            continue;
        if (!bh || BADNESS(tmp)<BADNESS(bh))
        {
            bh = tmp;
            if (!BADNESS(tmp))
                break;
        }
        /* and repeat until we find something good */
    }
    while ((tmp = tmp->b_next_free) != free_list);
    if (!bh)
    {
//tpc           sleep_on(&buffer_wait);
        goto repeat;
    }
    wait_on_buff(bh);
    if (bh->b_count)
        goto repeat;
    while (bh->b_dirt)
    {
        sync_dev(bh->b_dev);
        wait_on_buff(bh);
        if (bh->b_count)
            goto repeat;
    }
    /* NOTE!! While we slept waiting for this block, somebody else might */
    /* already have added "this" block to the cache. check it */
    if (find_buffer_from_hash(dev,block))
        goto repeat;
    /* OK, FINALLY we know that this buffer is the only one of it's kind, */
    /* and that it's unused (b_count=0), unlocked (b_lock=0), and clean */
    bh->b_count=1;
    bh->b_dirt=0;
    bh->b_uptodate=0;
    remove_from_queues(bh);
    bh->b_dev=dev;
    bh->b_blocknr=block;
    insert_into_queues(bh);
#ifdef _DEBUG_FLAG_BLK
    printk("debug_blk:get a new buff,dev:0x%x\n\r",bh->b_dev);
#endif
    return bh;
}

void put_buff(struct buffer_head * buf)
{
    if (!buf)
        return;
    wait_on_buff(buf);
    if (!(buf->b_count--))
        panic("Trying to free free buffer");
//tpc       wake_up(&buffer_wait);
}

/*
 * bread() reads a specified block and returns the buffer that contains
 * it. It returns NULL if the block was unreadable.
 */
struct buffer_head * read_buff(unsigned int dev, unsigned int block)
{
    struct buffer_head * bh;
#ifdef _DEBUG_FLAG_BLK
    puts("debug_blk:reading buff\n\r");
#endif
    if (!(bh=get_buff(dev,block)))
        panic("read_buff: get_buff returned NULL\n");
    if (bh->b_uptodate)
        return bh;
    ll_rw_block(READ,bh);
    wait_on_buff(bh);
    if (!bh->b_uptodate)
    {
       put_buff(bh);
       return NULL;
    }
    return bh;
}

#define COPYBLK(from,to) \
__asm__("cld\n\t" \
    "rep\n\t" \
    "movsl\n\t" \
    ::"c" (BLOCK_SIZE/4),"S" (from),"D" (to) \
    )

/*
 * bread_page reads four buffers into memory at the desired address. It's
 * a function of its own, as there is some speed to be got by reading them
 * all at the same time, not waiting for one to be read, and then another
 * etc.
 */
void read_buff_page(unsigned long address, unsigned int dev, unsigned int b[4])
{
    struct buffer_head * bh[4];
    int i;

    for (i=0 ; i<4 ; i++)
        if (b[i])
        {
            if ((bh[i] = get_buff(dev,b[i])))
                if (!bh[i]->b_uptodate)
                    ll_rw_block(READ,bh[i]);
        }
        else
            bh[i] = NULL;
    for (i=0 ; i<4 ; i++,address += BLOCK_SIZE)
        if (bh[i])
        {
            wait_on_buff(bh[i]);
            if (bh[i]->b_uptodate)
                COPYBLK((unsigned long) bh[i]->b_data,address);
            put_buff(bh[i]);
        }
}

/*
 * Ok, breada can be used as bread, but additionally to mark other
 * blocks for reading as well. End the argument list with a negative
 * number.
 */
struct buffer_head * read_buff_ahead(unsigned int dev, unsigned int first, ...)
{
    va_list args;
    struct buffer_head * bh, *tmp;

    va_start(args,first);
    if (!(bh=get_buff(dev,first)))
        panic("bread: get_buff returned NULL\n");
    if (!bh->b_uptodate)
        ll_rw_block(READ,bh);
    while ((first=va_arg(args,int))<MAX_HD)
    {
        tmp=get_buff(dev,first);
        if (tmp)
        {
            if (!tmp->b_uptodate)
                ll_rw_block(READA,tmp);   //this line: pre-read bocks after
            tmp->b_count--;               //this line: we won't use it immediately
        }
    }
    va_end(args);
    wait_on_buff(bh);
    if (bh->b_uptodate)
        return bh;
    put_buff(bh);
    return (NULL);
}

void init_buff(long buffer_end)
{
    struct buffer_head * h = start_buffer;
    void * b;
    int i;

    if (buffer_end == 1<<20)
        b = (void *) (640*1024);
    else
        b = (void *) buffer_end;
    while ( (b -= BLOCK_SIZE) >= ((void *) (h+1)) )
    {
        h->b_dev = 0;
        h->b_dirt = 0;
        h->b_count = 0;
        h->b_lock = 0;
        h->b_uptodate = 0;
        h->b_wait = NULL;
        h->b_next = NULL;
        h->b_prev = NULL;
        h->b_data = (char *) b;
        h->b_prev_free = h-1;
        h->b_next_free = h+1;
        h++;
        NR_BUFFERS++;
        if (b == (void *) 0x100000)
            b = (void *) 0xA0000;
    }
    h--;
    free_list = start_buffer;
    free_list->b_prev_free = h;
    h->b_next_free = free_list;
    for (i=0; i<NR_HASH; i++)
        hash_table[i]=NULL;
}

