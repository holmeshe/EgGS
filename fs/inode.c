#include <system.h>
#include <segment.h>
#include <fs.h>

struct m_inode inode_list[NR_INODE] ;

#define MAY_EXEC 1
#define MAY_WRITE 2
#define MAY_READ 4

static int permission(struct m_inode * inode,int mask)
{
//tpc   int mode = inode->i_mode;

    /* special case: not even root can read/write a deleted file */
    /*tpc   if (inode->i_dev && !inode->i_nlinks)
            return 0;
        else if (current->euid==inode->i_uid)
            mode >>= 6;
        else if (current->egid==inode->i_gid)
            mode >>= 3;
        if (((mode & mask & 0007) == mask) || suser())
            return 1;
        return 0;*/
    return 1;
}


inline void wait_on_inode(struct m_inode * inode)
{
#ifdef _DEBUG_FLAG_LOCK
    printk("wait on inode[%d]\n", inode->b_lock);
#endif

    cli();
    while (inode->i_lock)
//tpc                           sleep_on(&inode->i_wait);
        sti();
}

inline void lock_inode(struct m_inode * inode)
{
#ifdef _DEBUG_FLAG_LOCK
    printk("debug_blk:lock inode lock[%d]\n", bh->b_lock);
#endif

    cli();
    while (inode->i_lock)
//tpc                           sleep_on(&inode->i_wait);
        inode->i_lock=1;
    sti();
}

inline void unlock_inode(struct m_inode * inode)
{
#ifdef _DEBUG_FLAG_LOCK
    printk("debug_blk:unlock inode lock[%d]\n", bh->b_lock);
#endif

    inode->i_lock=0;
//tpc                       wake_up(&inode->i_wait);
}

static void read_inode_from_buff(struct m_inode * inode)
{
    struct super_block * sb;
    struct buffer_head * bh;
    int block;

    lock_inode(inode);
    if (!(sb=find_super(inode->i_dev)))
        panic("trying to read inode without dev");
    block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks +
            (inode->i_num-1)/INODES_PER_BLOCK;
    if (!(bh=read_buff(inode->i_dev,block)))
        panic("unable to read i-node block");
    *(struct d_inode *)inode =
        ((struct d_inode *)bh->b_data)
        [(inode->i_num-1)%INODES_PER_BLOCK];
    put_buff(bh);
    unlock_inode(inode);
}

static void sync_inode_to_buff(struct m_inode * inode)
{
    struct super_block * sb;
    struct buffer_head * bh;
    int block;

#ifdef _DEBUG_FLAG_FS
    printk("debug_fs:sync inode[0x%x:0x%x]\n", inode->i_dev, inode->i_num);
#endif
    lock_inode(inode);
    if (!inode->i_dirt || !inode->i_dev)
    {
        unlock_inode(inode);
        return;
    }
    if (!(sb=find_super(inode->i_dev)))
        panic("trying to write inode without device");
    block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks +
            (inode->i_num-1)/INODES_PER_BLOCK;
    if (!(bh=read_buff(inode->i_dev,block)))
        panic("unable to read i-node block");
    ((struct d_inode *)bh->b_data)
    [(inode->i_num-1)%INODES_PER_BLOCK] =
        *(struct d_inode *)inode;
    bh->b_dirt=1;
    inode->i_dirt=0;
    put_buff(bh);
    unlock_inode(inode);
}

void sync_all_inodes_to_buff(void)
{
    int i;
    struct m_inode * inode;

#ifdef _DEBUG_FLAG_FS
    printk("debug_fs:sync all inodes\n");
#endif
    inode = 0+inode_list;
    for(i=0 ; i<NR_INODE ; i++,inode++)
    {
        wait_on_inode(inode);
        if (inode->i_dirt && !inode->i_pipe)
            sync_inode_to_buff(inode);
    }
}

#define BADNESS(inode) (((inode)->i_dirt<<1)+(inode)->i_lock)
static struct m_inode * new_inode(unsigned int dev, unsigned int nr)
{
    struct m_inode * inode;
    static struct m_inode * last_inode = inode_list;
    int i;

#ifdef _DEBUG_FLAG_FS
        printk("debug_fs: in new_inode\n");
#endif

    inode = NULL;
    for (i = 0; i < NR_INODE ; i++)
    {
        if (++last_inode >= inode_list + NR_INODE)
            last_inode = inode_list;
        if (last_inode->i_count == 0 &&
            (!inode || BADNESS(last_inode)<BADNESS(inode)))
        {
            inode = last_inode;
            if (!BADNESS(inode))
                break;
        }
    }

    if (!inode)
    {
        for (i=0 ; i<NR_INODE ; i++)
            printk("%04x: %6d\t",inode_list[i].i_dev,
                   inode_list[i].i_num);
        panic("No free inodes in mem");
    }

    while (inode->i_dirt)
    {
        wait_on_inode(inode);
        sync_inode_to_buff(inode);
        if (inode->i_count != 0)
            return NULL;
    }

    if (find_inode(&dev, &nr))
    {
        printk("inode bad luck");
        return NULL;
    }
    memset(inode,0,sizeof(*inode));
    inode->i_count = 1;
    inode->i_dev = dev;
    inode->i_num = nr;
#ifdef _DEBUG_FLAG_FS
    printk("debug_fs:new inode[0x%x:0x%x]\n\r",inode->i_dev, inode->i_num);
#endif
    return inode;
}

struct m_inode * find_inode(unsigned int * dev, unsigned int * nr)
{
    struct m_inode * inode;

    if (*dev == 0)
        panic("find inode with dev==0");

#ifdef _DEBUG_FLAG_FS
    printk("debug_fs: in find_inode[0x%x:0x%x]\n\r", *dev, *nr);
#endif

    inode = inode_list;
    while (inode < NR_INODE+inode_list)
    {
        if (inode->i_dev != *dev || inode->i_num != *nr)
        {
            inode++;
            continue;
        }

        if (inode->i_mount)
        {
            int i;

            for (i = 0 ; i<NR_SUPER ; i++)
                if (super_block_list[i].s_imount==inode)
                    break;
            if (i >= NR_SUPER)
            {
                printk("Mounted inode hasn't got sb\n");
                printk("debug_fs:find inode[0x%x:0x%x]\n\r", dev, nr);
                return inode;
            }

            *dev = super_block_list[i].s_dev;
            *nr = ROOT_INO;
            inode = inode_list;
            continue;
        }
#ifdef _DEBUG_FLAG_FS
        printk("debug_fs:found inode[0x%x:0x%x]\n\r", *dev, *nr);
#endif

        return inode;
    }

    return NULL;
}

struct m_inode * retain_inode(unsigned int * dev, unsigned int * nr)
{
    struct m_inode * inode = NULL;


    if (*dev == 0)
        panic("retain inode with dev==0");

    for (;;)
    {
        //the actuall dev and nr can be chaned in find_inode
        if (!(inode=find_inode(dev,nr)))
        {
            return NULL;
        }

        wait_on_inode(inode);
        if (inode->i_dev != *dev || inode->i_num != *nr)
        {
            continue;
        }
        inode->i_count++;
#ifdef _DEBUG_FLAG_FS
        printk("debug_fs:retained inode[0x%x:0x%x]\n\r", *dev, *nr);
#endif
        return inode;
    }

    return NULL;
}

struct m_inode * get_inode(unsigned int dev, unsigned int nr)
{
    struct m_inode * inode = NULL;
    do
    {
        if ((inode = retain_inode(&dev, &nr)) == NULL)
            inode = new_inode(dev, nr);
    }
    while (inode == NULL);

    return inode;
}

void put_inode(struct m_inode * inode)
{
    if (!inode)
        return;

    wait_on_inode(inode);
    if (0 == inode->i_count)
        panic("iput: trying to free free inode");
repeat:
    if (inode->i_count == 1)
    {
        if (!inode->i_nlinks)
        {
            truncate(inode);
            delete_inode(inode);
            return;
        }
        if (inode->i_dirt)
        {
            sync_inode_to_buff(inode);
            wait_on_inode(inode);
            goto repeat;//this line: when we sleep, someone may change it
        }
    }
    inode->i_count--;
    return;
}

struct m_inode * read_inode(int dev,int nr)
{
    struct m_inode * inode;

#ifdef _DEBUG_FLAG_FS
    printk("debug_fs:read inode[0x%x:0x%x]\n", dev, nr);
#endif
    inode = get_inode(dev, nr);

    if (inode->i_nlinks == 0)
        read_inode_from_buff(inode);

    return inode;
}

void delete_inode(struct m_inode * inode)
{
    if (!inode)
        return;
    if (!inode->i_dev)
    {
        memset(inode,0,sizeof(*inode));
        return;
    }
    if (inode->i_count>1)
    {
        printk("trying to free inode with count=%d\n",inode->i_count);
        panic("free_inode");
    }
    if (inode->i_nlinks)
        panic("trying to free inode with links");

    free_inode_bitmap(inode->i_dev, inode->i_num);

    memset(inode,0,sizeof(*inode));
}

struct m_inode * create_inode(unsigned int dev)
{
    struct m_inode * inode;
    int i_num;


    if ((i_num = add_inode_bitmap(dev)) == 0)
        return NULL;

    if ((inode=new_inode(dev, i_num)) == NULL)
        return NULL;

    inode->i_nlinks=1;
//tpc                               inode->i_uid=current->euid;
//tpc                               inode->i_gid=current->egid;
    inode->i_dirt=1;
//tpc                               inode->i_mtime = inode->i_atime = inode->i_ctime = CURRENT_TIME;
    return inode;
}

int locate_by_inode(struct m_inode * inode,int block,int create)
{
    struct buffer_head * bh;
    int i;

    if (block<0)
        panic("locate_in_inode: block<0");
    if (block >= 7+512+512*512)
        panic("locate_in_inode: block>big");
    if (block<7)
    {
        if (create && !inode->i_zone[block])
            if ((inode->i_zone[block]=create_block(inode->i_dev)))
            {
//tpc                     inode->i_ctime=CURRENT_TIME;
                inode->i_dirt=1;
            }
        return inode->i_zone[block];
    }
    block -= 7;
    if (block<512)
    {
        if (create && !inode->i_zone[7])
        {
            if ((inode->i_zone[7]=create_block(inode->i_dev)))
            {
                inode->i_dirt=1;
//tpc               inode->i_ctime=CURRENT_TIME;
            }
        }
        if (!inode->i_zone[7])
            return 0;
        if (!(bh = read_buff(inode->i_dev,inode->i_zone[7])))
            return 0;
        i = ((unsigned short *) (bh->b_data))[block];
        if (create && !i)
            if ((i=create_block(inode->i_dev)))
            {
                ((unsigned short *) (bh->b_data))[block]=i;
                bh->b_dirt=1;
            }
        put_buff(bh);
        return i;
    }
    block -= 512;
    if (create && !inode->i_zone[8])
        if ((inode->i_zone[8]=create_block(inode->i_dev)))
        {
            inode->i_dirt=1;
//tpc           inode->i_ctime=CURRENT_TIME;
        }
    if (!inode->i_zone[8])
        return 0;
    if (!(bh=read_buff(inode->i_dev,inode->i_zone[8])))
        return 0;
    i = ((unsigned short *)bh->b_data)[block>>9];
    if (create && !i)
    {
        if ((i=create_block(inode->i_dev)))
        {
            ((unsigned short *) (bh->b_data))[block>>9]=i;
            bh->b_dirt=1;
        }
    }
    put_buff(bh);
    if (!i)
        return 0;
    if (!(bh=read_buff(inode->i_dev,i)))
        return 0;
    i = ((unsigned short *)bh->b_data)[block&511];
    if (create && !i)
        if ((i=create_block(inode->i_dev)))
        {
            ((unsigned short *) (bh->b_data))[block&511]=i;
            bh->b_dirt=1;
        }
    put_buff(bh);
    return i;
}

void truncate(struct m_inode * inode)
{
    int i;

    if (!(S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode)))
        return;
    for (i=0; i<7; i++)
        if (inode->i_zone[i])
        {
            delete_block(inode->i_dev,inode->i_zone[i]);
            inode->i_zone[i]=0;
        }
    delete_indirecte_block(inode->i_dev,inode->i_zone[7]);
    delete_double_indirecte_block(inode->i_dev,inode->i_zone[8]);
    inode->i_zone[7] = inode->i_zone[8] = 0;
    inode->i_size = 0;
    inode->i_dirt = 1;
//tpc   inode->i_mtime = inode->i_ctime = CURRENT_TIME;
}

struct m_inode * retain_inode_by_name(struct m_inode ** inode,
                                      const char * name, int namelen)
{
    int entries;
    int block,i;
    struct buffer_head * bh;
    struct dir_entry * de, * res;
    struct super_block * sb;
#ifdef _DEBUG_FLAG_FS
    printk("debug_fs:retain inode by name[%.*s] in inode[%d]\n",namelen, name, (*inode)->i_num);
#endif
    if (namelen > NAME_LEN)
        namelen = NAME_LEN;
    entries = (*inode)->i_size / (sizeof (struct dir_entry));

    if (!namelen)
        return NULL;

    if (namelen==2 && get_fs_byte(name)=='.' && get_fs_byte(name+1)=='.')
    {
//tpc       if (inode == current->root)
//tpc           namelen=1;
        if ((*inode)->i_num == ROOT_INO)
        {
            sb=find_super((*inode)->i_dev);
            if (sb->s_imount)
            {
                put_inode(*inode);
                (*inode)=sb->s_imount;
                (*inode)->i_count++;
            }
        }
    }
    if (!(block = (*inode)->i_zone[0]))
        return NULL;
    if (!(bh = read_buff((*inode)->i_dev,block)))
        return NULL;
    i = 0;

    de = (struct dir_entry *) bh->b_data;
    res = NULL;
#ifdef _DEBUG_FLAG_FS
            printk("debug_fs:entries[");
#endif

    while (i < entries)
    {
        if ((char *)de >= BLOCK_SIZE+bh->b_data)
        {
            put_buff(bh);
            bh = NULL;
            if (!(block = locate_by_inode(*inode, i/DIR_ENTRIES_PER_BLOCK, 0)) ||
                !(bh = read_buff((*inode)->i_dev,block)))
            {
                i += DIR_ENTRIES_PER_BLOCK;
                continue;
            }
            de = (struct dir_entry *) bh->b_data;
        }
#ifdef _DEBUG_FLAG_FS
        printk("%.*s,",namelen, de->name);
#endif

        if (match(namelen,name,de->name))
        {
#ifdef _DEBUG_FLAG_FS
        printk("found! inode[0x%x]",de->inode);
#endif

            res = de;
            break;
        }
        de++;
        i++;
    }
#ifdef _DEBUG_FLAG_FS
    printk("]\n");
#endif

    put_buff(bh);
    if (res)
    {
        return read_inode((*inode)->i_dev, res->inode);
    }
    return NULL;
}

struct m_inode * retain_inode_by_path(struct m_inode * inode, const char * pathname)
{
    char c;
    const char * thisname;
    struct m_inode * res_inode, * tmp;
    int namelen;

    if (!(c=get_fs_byte(pathname)))
      return NULL;

    if ((c=get_fs_byte(pathname))=='/')
    {
        pathname++;
    }

    tmp = NULL;
    
    res_inode = inode;
    res_inode->i_count++;   //for we can put_inode uniformly after retain_inode_by_name
    namelen = 0;
    thisname = pathname;
    while ((c=get_fs_byte(pathname++)) != '\0')
    {
        if (c!='/')
        {
            namelen++;
            continue;
        }
        
        if (!S_ISDIR(inode->i_mode) || !permission(inode,MAY_EXEC))
        {
            return NULL;
        }
#ifdef _DEBUG_FLAG_FS
//        printk("debug_fs:parsed name:%s, len:%d, inode:%d\n", thisname, namelen, res_inode->i_num);
#endif
        if (!(tmp = retain_inode_by_name(&res_inode,thisname,namelen)))
        {
            return NULL;
        }
        put_inode(res_inode);
        res_inode = tmp;
        thisname = pathname;
        namelen = 0;
    }

    return res_inode;
}


struct m_inode * create_inode_by_name(struct m_inode * inode, const char * name, int namelen)
{
    int block,i;
    struct m_inode * new_inode;
    struct buffer_head * bh;
    struct dir_entry * de;

    if (namelen > NAME_LEN)
        namelen = NAME_LEN;
    if (!namelen)
        return NULL;
    if (!(block = inode->i_zone[0]))
        return NULL;
    if (!(bh = read_buff(inode->i_dev,block)))
        return NULL;
    i = 0;
    de = (struct dir_entry *) bh->b_data;
    while (1)
    {
        if ((char *)de >= BLOCK_SIZE+bh->b_data)
        {
            put_buff(bh);
            bh = NULL;
            block = locate_by_inode(inode, i/DIR_ENTRIES_PER_BLOCK, 1);
            if (!block)
                return NULL;
            if (!(bh = read_buff(inode->i_dev,block)))
            {
                i += DIR_ENTRIES_PER_BLOCK;
                continue;
            }
            de = (struct dir_entry *) bh->b_data;
        }
        if (i*sizeof(struct dir_entry) >= inode->i_size)
        {
            de->inode=0;
            inode->i_size = (i+1)*sizeof(struct dir_entry);
            inode->i_dirt = 1;
//tpc           inode->i_ctime = CURRENT_TIME;
        }
        if (!de->inode)
        {
//tpc           dir->i_mtime = CURRENT_TIME;
            new_inode = create_inode(inode->i_dev);
            if (!new_inode)
            {
                put_buff(bh);
                return NULL;
            }
//tpc           new_inode->i_uid = current->euid;
            new_inode->i_dirt = 1;
            de->inode = new_inode->i_num;

            for (i=0; i < NAME_LEN ; i++)
                de->name[i]=(i<namelen)?get_fs_byte(name+i):0;
            bh->b_dirt = 1;
            put_buff(bh);
            return new_inode;
        }
        de++;
        i++;
    }
    put_buff(bh);
    return NULL;
}


#ifdef _DEBUG_FLAG_FS
void test_inode()
{
//    struct m_inode * inode = create_inode(0x301);
//    sync_all_inodes_to_buff();
//    sync_dev(0x301);
//    inode->i_nlinks = 0;
//    delete_inode(inode);
//    sync_all_inodes_to_buff();
//    sync_dev(0x301);

//      struct m_inode * inode = read_inode(0x301, 1);
//      struct m_inode * inode_2 = retain_inode_by_name(&inode, "hello.txt", 9);
//      printk("found file:%x\n", inode_2->i_num);

      struct m_inode * inode = read_inode(0x301, 1);
      struct m_inode * inode_2 = retain_inode_by_path(inode, "123/456/789/aaa");
      if (inode_2 != NULL)
            printk("found path:%x\n", inode_2->i_num);
}
#endif

