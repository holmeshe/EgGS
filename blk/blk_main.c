/*
 * This handles all read/write requests to block devices
 */
#include <multiboot.h>
#include <system.h>
#include "blk_main.h"

/*
 * The request-struct contains all necessary data
 * to load a nr of sectors into memory
 */
struct request request_list[NR_REQUEST];

/*
 * used to wait on when there are no free requests
 */
//tpc struct task_struct * wait_for_request = NULL;

/* blk_dev_struct is:
 *  do_request-address
 *  next-request
 */
struct blk_dev_struct blk_dev[NR_BLK_DEV] =
{
    { NULL, NULL },     /* no_dev */
    { NULL, NULL },     /* dev mem */
    { NULL, NULL },     /* dev fd */
    { NULL, NULL },     /* dev hd */
    { NULL, NULL },     /* dev ttyx */
    { NULL, NULL },     /* dev tty */
    { NULL, NULL }      /* dev lp */
};

/*
 * add-request adds a request to the linked list.
 * It disables interrupts so that it can muck with the
 * request-lists in peace.
 */
static void add_request(struct blk_dev_struct * dev, struct request * req)
{
    struct request * tmp;

    req->next = NULL;
    cli();        //kernel can not be scheduled, can this line be deleted?
    if (req->bh)
        req->bh->b_dirt = 0;  //this line: clear dirt so early?

    if (!(tmp = dev->current_request)) //this line: the drive is stopped
    {
        dev->current_request = req;
        sti();
        (dev->request_fn)();

    }
    else    //this line: the drive is running, we just add the request on the queue
    {
        //this line: elevator
        for ( ; tmp->next ; tmp=tmp->next)
            if ((IN_ORDER(tmp,req) ||
                 !IN_ORDER(tmp,tmp->next)) &&
                IN_ORDER(req,tmp->next))
                break;
        req->next=tmp->next;
        tmp->next=req;
        sti();
    }
    
    return;
}

void ll_rw_block(int rw, struct buffer_head * bh)
{
    unsigned int major;
    struct request * req;
    int rw_ahead;

    if ((major=MAJOR(bh->b_dev)) >= NR_BLK_DEV ||
        !(blk_dev[major].request_fn))
    {
        printk("Trying to read nonexistent block-device\n\r");
        return;
    }


    /* WRITEA/READA is special case - it is not really needed, so if the */
    /* buffer is locked, we just forget about it, else it's a normal read */
    if ((rw_ahead = (rw == READA || rw == WRITEA)))
    {
        if (bh->b_lock)
            return;
        if (rw == READA)
            rw = READ;
        else
            rw = WRITE;
    }
    if (rw!=READ && rw!=WRITE)
        panic("Bad block dev command, must be R/W/RA/WA");
    lock_buff(bh);
    if ((rw == WRITE && !bh->b_dirt) || (rw == READ && bh->b_uptodate))
    {
        printk("read|write:%d dirt:%d uptodate:%d\n\r", rw, bh->b_dirt, bh->b_uptodate);
        unlock_buff(bh);
        return;
    }
repeat:
    /* we don't allow the write-requests to fill up the queue completely:
     * we want some room for reads: they take precedence. The last third
     * of the requests are only for reads.
     */
    if (rw == READ)
        req = request_list+NR_REQUEST;
    else
        req = request_list+((NR_REQUEST*2)/3);
    /* find an empty request */
    while (--req >= request_list)
        if (req->dev<0)
            break;
    /* if none found, sleep on new requests: check for rw_ahead */
    if (req < request_list)
    {
        if (rw_ahead)
        {
            unlock_buff(bh);
            return;
        }
#ifdef _DEBUG_FLAG_BLK
        printk("debug_blk:request queue full, sleep\n\r");
#endif
//tpc               sleep_on(&wait_for_request);

        goto repeat;
    }
    /* fill up the request-info, and add it to the queue */
    req->dev = bh->b_dev;
    req->cmd = rw;
    req->errors=0;
    req->sector = bh->b_blocknr<<1;
    req->nr_sectors = 2;
    req->buffer = bh->b_data;
//tpc                   req->waiting = NULL;
    req->bh = bh;
    req->next = NULL;
#ifdef _DEBUG_FLAG_BLK
    printk("debug_blk:---add request queue major:%d dev:0x%x cmd:%d sector:%d\n\r", 
            major, req->dev, req->cmd, req->sector);
#endif
    add_request(&blk_dev[major],req);
}


void init_request_queue(void)
{
    int i;

    for (i=0 ; i<NR_REQUEST ; i++)
    {
        request_list[i].dev = -1;
        request_list[i].next = NULL;
    }
}


void init_blk(multiboot_info_t * mbi)
{
      init_buff(2*1024*1024);
      init_request_queue();

      if (mbi->flags & 1 << 6)
      {
         printk ("init hd info, size: %d\n", mbi->drives_length);
         init_hd_info(mbi->drives_addr, mbi->drives_length);
      }
}


