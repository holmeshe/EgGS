#include <system.h>
#include <fs.h>
#include "hdreg.h"


#define MAJOR_NR 3
#include "blk_main.h"


/* Max read/write errors/sector */
#define MAX_ERRORS  7


static void recal_intr(void);
static void read_intr(void);
static void write_intr(void);
static void enable_hd(void);



static int recalibrate = 1;
static int reset = 1;

/*
 *  This struct defines the HD's and their types.
 */
struct hd_i_struct
{
    int head,sect,cyl;
};
struct hd_i_struct hd_info[MAX_HD] = { {0,0,0},{0,0,0} };
static unsigned int NR_HD = 0;

static struct partition_i_struct
{
    unsigned long start_sect;
    unsigned long nr_sects;
} partition_info[5*MAX_HD]= {{0,0},};

#define port_read(port,buf,nr) \
__asm__("cld;rep;insw"::"d" (port),"D" (buf),"c" (nr))

#define port_write(port,buf,nr) \
__asm__("cld;rep;outsw"::"d" (port),"S" (buf),"c" (nr))


static inline void end_request(int uptodate)
{
#ifdef _DEBUG_FLAG_BLK
    printk("debug_blk:---end request dev[0x%x] uptodate[%d]\n\r",CURRENT_REQUEST->dev,uptodate);
#endif

    DEVICE_OFF(CURRENT_REQUEST->dev);
    if (CURRENT_REQUEST->bh)
    {
        CURRENT_REQUEST->bh->b_uptodate = uptodate;
        unlock_buff(CURRENT_REQUEST->bh);
    }
    if (!uptodate)
    {
        printk(DEVICE_NAME " I/O error\n\r");
        printk("dev %04x, block %d\n\r",CURRENT_REQUEST->dev,
               CURRENT_REQUEST->bh->b_blocknr);
    }
//tpc   wake_up(&CURRENT->waiting);
//tpc   wake_up(&wait_for_request);
    CURRENT_REQUEST->dev = -1;
    CURRENT_REQUEST = CURRENT_REQUEST->next;
}


int init_hd_info(unsigned int drives_addr, unsigned int drives_length)
{
    static int callable = 1;
    unsigned int i,drive;
    struct partition *p;
    struct buffer_head * bh;

    enable_hd();

    if (!callable)
        return -1;
    callable = 0;

    unsigned int size = 0;

    for (drive=0 ; drive<MAX_HD ; drive++)
    {
        hd_info[drive].cyl = *(unsigned short *) (6+drives_addr);
        hd_info[drive].head = *(unsigned char *) (8+drives_addr);
        hd_info[drive].sect = *(unsigned char *) (9+drives_addr);
        size += *(unsigned int *)drives_addr;
        printk("hd%d: cylinders[%d] heads[%d] spt[%d]\n",
               drive, hd_info[drive].cyl, hd_info[drive].head, hd_info[drive].sect);

        NR_HD++;
        if (size > drives_length)
        {
            drives_addr += *(unsigned int *)drives_addr;
        }
        else
        {
            break;
        }

    }

    for (i=0 ; i<NR_HD ; i++)
    {
        partition_info[i*5].start_sect = 0;
        partition_info[i*5].nr_sects = hd_info[i].head*
                                       hd_info[i].sect*hd_info[i].cyl;
    }

    for (drive=0 ; drive<NR_HD ; drive++)
    {
        if (!(bh = read_buff(0x300 + drive*5,0)))
        {
            printk("Unable to read partition table of drive %d\n\r",
                   drive);
            panic("");
        }
        if (bh->b_data[510] != 0x55 || (unsigned char)
            bh->b_data[511] != 0xAA)
        {
            printk("Bad partition table on drive %d\n\r",drive);
            panic("");
        }
        p = 0x1BE + (void *)bh->b_data;
        for (i=1; i<5; i++,p++)
        {
            partition_info[i+5*drive].start_sect = p->start_sect;
            partition_info[i+5*drive].nr_sects = p->nr_sects;
            if (partition_info[i+5*drive].start_sect > 0)
                printk("hd%d partition%d: start_sect[%d] nr_sects[%d]\n",
                       drive, i, partition_info[i+5*drive].start_sect, partition_info[i+5*drive].nr_sects);
        }
        put_buff(bh);
    }

    return (0);
}

static int win_result(void)
{
    int i=inb_p(HD_STATUS);

    if ((i & (BUSY_STAT | READY_STAT | WRERR_STAT | SEEK_STAT | ERR_STAT))
        == (READY_STAT | SEEK_STAT))
        return(0); /* ok */
    if (i&1) i=inb(HD_ERROR);
    return (1);
}

static int controller_ready(void)
{
    int retries=1000000;

    while (--retries && (inb_p(HD_STATUS)&(BUSY_STAT|READY_STAT))!=READY_STAT);
    return (retries);
}


static void hd_out(unsigned int drive,unsigned int nsect,unsigned int sect,
                   unsigned int head,unsigned int cyl,unsigned int cmd,
                   void (*intr_addr)(void))
{
    register int port;

    if (drive>1 || head>15)
        panic("Trying to write bad sector");
    if (!controller_ready())
        panic("HD controller not ready");
    irq_install_handler(14, intr_addr);
    outb_p(0,HD_CMD);
    port=HD_DATA;
    outb_p(0,++port);    //if1:Features / Error Information(Usually used for ATAPI devices.)
    outb_p(nsect,++port);//if2:Sector Count
    outb_p(sect,++port); //if3:Sector Number / LBAlo
    outb_p(cyl,++port);  //if4:Cylinder Low / LBAmid
    outb_p(cyl>>8,++port);//if5:Cylinder High / LBAhi
    outb_p(0xA0|(drive<<4)|head,++port);//if6:Drive / Head Port
    outb(cmd,++port);    //if7:Command port / Regular Status port
}


static int drive_busy(void)
{
    unsigned int i;

    for (i = 0; i < 10000; i++)
        if (READY_STAT == (inb_p(HD_STATUS) & (BUSY_STAT|READY_STAT)))
            break;
    i = inb(HD_STATUS);
    i &= BUSY_STAT | READY_STAT | SEEK_STAT;
    if (i == (READY_STAT | SEEK_STAT))
        return(0);
    printk("HD controller times out\n\r");
    return(1);
}

static void reset_controller(unsigned int dev)
{
    int i;

    outb(4,HD_CMD);
    for(i = 0; i < 100; i++) nop();
    outb(0 ,HD_CMD);
    if (drive_busy())
        printk("HD-controller still busy\n\r");
    if ((i = inb(HD_ERROR)) != 1)
        printk("HD-controller reset failed: %02x\n\r",i);
    hd_out(dev,hd_info[dev].sect,hd_info[dev].sect,hd_info[dev].head-1,
           hd_info[dev].cyl,WIN_SPECIFY,&recal_intr); //in bochs, this line causes 
           //'init drive params: max. logical head number 0 not supported'
           //still don't know why

}

static void bad_rw_intr(void)
{
    if (++CURRENT_REQUEST->errors >= MAX_ERRORS)
        end_request(0);
    if (CURRENT_REQUEST->errors > MAX_ERRORS/2)
        reset = 1;
}

void do_hd_request(void)
{
    int i,r = 0;
    unsigned int block;
    unsigned int sec,head,cyl;
    unsigned int nsect;
    unsigned int drive, partition;

repeat:
    if (!CURRENT_REQUEST)
        return;

    if (MAJOR(CURRENT_REQUEST->dev) != MAJOR_NR)
        panic(DEVICE_NAME ": request list destroyed");

    if (CURRENT_REQUEST->bh)
    {
        if (!CURRENT_REQUEST->bh->b_lock)
            panic(DEVICE_NAME ": block not locked");
    }

    partition = MINOR(CURRENT_REQUEST->dev);
    drive = partition / 5;
    block = CURRENT_REQUEST->sector;
    if (partition >= NR_HD*5 || block+2 > partition_info[partition].nr_sects)
    {
        end_request(0);
        goto repeat;
    }
    block += partition_info[partition].start_sect;
#ifdef _DEBUG_FLAG_BLK
    printk("debug_blk:info dev[0x%x] partition[%d] offset[%d] drive[0x%x] sector[%d]\n\r",
    CURRENT_REQUEST->dev, partition, partition_info[partition].start_sect, drive, block);
#endif

    __asm__("divl %4":"=a" (block),"=d" (sec):"0" (block),"1" (0),
            "r" (hd_info[drive].sect));                             //this line: chs
    __asm__("divl %4":"=a" (cyl),"=d" (head):"0" (block),"1" (0),
            "r" (hd_info[drive].head));                             //this line: chs
    sec++;

    if (reset)
    {
#ifdef _DEBUG_FLAG_BLK
        printk("debug_blk:dev[0x%x] reset\n\r",CURRENT_REQUEST->dev);
#endif
        reset = 0;
        recalibrate = 1;
        reset_controller(drive);

        return;
    }
    if (recalibrate)
    {
#ifdef _DEBUG_FLAG_BLK
        printk("debug_blk:dev[0x%x] recalibrate\n\r",CURRENT_REQUEST->dev);
#endif
        recalibrate = 0;
        hd_out(drive,hd_info[drive].sect,0,0,0,
               WIN_RESTORE,&recal_intr);
        return;
    }


    nsect = CURRENT_REQUEST->nr_sectors;


    if (CURRENT_REQUEST->cmd == WRITE)
    {
#ifdef _DEBUG_FLAG_BLK
        printk("debug_blk:write to dev[0x%x]\n\r",CURRENT_REQUEST->dev);
#endif

        hd_out(drive,nsect,sec,head,cyl,WIN_WRITE,&write_intr);
        for(i=0 ; i<3000 && !(r=inb_p(HD_STATUS)&DRQ_STAT) ; i++)
            /* nothing */ ;
        if (!r)
        {
            bad_rw_intr();
            printk("write failed dev:0x%x times:%d\n\r",CURRENT_REQUEST->dev,CURRENT_REQUEST->errors);
            goto repeat;
        }
        port_write(HD_DATA,CURRENT_REQUEST->buffer,256);//after this line, the controller will do its job and then trigger an interrupt
    }
    else if (CURRENT_REQUEST->cmd == READ)
    {
#ifdef _DEBUG_FLAG_BLK
        printk("debug_blk:read from dev[0x%x]\n\r",CURRENT_REQUEST->dev);
#endif

        hd_out(drive,nsect,sec,head,cyl,WIN_READ,&read_intr);
    }
    else
        panic("unknown hd-command");
}


static void read_intr(void)
{
    if (win_result())
    {
        bad_rw_intr();
        printk("read failed dev:0x%x times:%d\n\r",CURRENT_REQUEST->dev,CURRENT_REQUEST->errors);
        do_hd_request();
        return;
    }
    port_read(HD_DATA,CURRENT_REQUEST->buffer,256);
    CURRENT_REQUEST->errors = 0;
    CURRENT_REQUEST->buffer += 512;
    CURRENT_REQUEST->sector++;
    if (--CURRENT_REQUEST->nr_sectors)
    {
#ifdef _DEBUG_FLAG_BLK
        printk("debug_blk:more read from dev[0x%x]\n\r",CURRENT_REQUEST->dev);
#endif

        irq_install_handler(14, &read_intr);
        return;
    }

#ifdef _DEBUG_FLAG_BLK
            printk("debug_blk:done reading dev[0x%x]\n\r",CURRENT_REQUEST->dev);
#endif

    end_request(1);
    do_hd_request();
}

static void write_intr(void)
{
    if (win_result())
    {
        bad_rw_intr();
        printk("write failed dev:0x%x times:%d\n\r",CURRENT_REQUEST->dev,CURRENT_REQUEST->errors);
        do_hd_request();
        return;
    }
    if (--CURRENT_REQUEST->nr_sectors)
    {
#ifdef _DEBUG_FLAG_BLK
        printk("debug_blk:more write to dev[0x%x]\n\r",CURRENT_REQUEST->dev);
#endif
        CURRENT_REQUEST->sector++;
        CURRENT_REQUEST->buffer += 512;
        irq_install_handler(14, &write_intr);
        port_write(HD_DATA,CURRENT_REQUEST->buffer,256);
        return;
    }

#ifdef _DEBUG_FLAG_BLK
            printk("debug_blk:done write dev[0x%x]\n\r",CURRENT_REQUEST->dev);
#endif
    
    end_request(1);
    do_hd_request();
}

static void recal_intr(void)
{
    if (win_result())
    {
        bad_rw_intr();
        printk("reset failed dev:0x%x times:%d\n\r",CURRENT_REQUEST->dev, CURRENT_REQUEST->errors);
    }
#ifdef _DEBUG_FLAG_BLK
        printk("debug_blk:done reset dev[0x%x]\n\r",CURRENT_REQUEST->dev);
#endif

    do_hd_request();
}

static void enable_hd(void)
{
    blk_dev[MAJOR_NR].request_fn = do_hd_request;     //init hd request handler for ll_rw_blk
    outb_p(inb_p(0x21)&0xfb,0x21);
    outb(inb_p(0xA1)&0xbf,0xA1);
}

