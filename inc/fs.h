/*
 * This file has definitions for some important file table
 * structures etc.
 */

#ifndef _FS_H
#define _FS_H

#include <multiboot.h>


/* devices are as follows: (same as minix, so we can use the minix
 * file system. These are major numbers.)
 *
 * 0 - unused (nodev)
 * 1 - /dev/mem
 * 2 - /dev/fd
 * 3 - /dev/hd
 * 4 - /dev/ttyx
 * 5 - /dev/tty
 * 6 - /dev/lp
 * 7 - unnamed pipes
 */

#define MAX_HD      2

#define IS_SEEKABLE(x) ((x)>=1 && (x)<=3)

#define READ 0
#define WRITE 1
#define READA 2		/* read-ahead - don't pause */
#define WRITEA 3	/* "write-ahead" - silly, but somewhat useful */

#define MAJOR(a) (((unsigned)(a))>>8)
#define MINOR(a) ((unsigned)(a)&0xff)

#define NAME_LEN 30
#define ROOT_INO 1

#define I_MAP_SLOTS 8
#define Z_MAP_SLOTS 8
#define SUPER_MAGIC 0x138F

#define NR_OPEN 20
#define NR_INODE 32
#define NR_FILE 64
#define NR_SUPER 8
#define NR_HASH 307
#define NR_BUFFERS nr_buffers
#define BLOCK_SIZE 1024
#define BLOCK_SIZE_BITS 10
#ifndef NULL
#define NULL ((void *) 0)
#endif

#define INODES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct d_inode)))
#define DIR_ENTRIES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct dir_entry)))

#define PIPE_HEAD(inode) ((inode).i_zone[0])
#define PIPE_TAIL(inode) ((inode).i_zone[1])
#define PIPE_SIZE(inode) ((PIPE_HEAD(inode)-PIPE_TAIL(inode))&(PAGE_SIZE-1))
#define PIPE_EMPTY(inode) (PIPE_HEAD(inode)==PIPE_TAIL(inode))
#define PIPE_FULL(inode) (PIPE_SIZE(inode)==(PAGE_SIZE-1))
#define INC_PIPE(head) \
__asm__("incl %0\n\tandl $4095,%0"::"m" (head))

typedef char buffer_block[BLOCK_SIZE];

struct buffer_head {
	char * b_data;			/* pointer to data block (1024 bytes) */
	unsigned long b_blocknr;	/* block number */
	unsigned short b_dev;		/* device (0 = free) */
	unsigned char b_uptodate;
	unsigned char b_dirt;		/* 0-clean,1-dirty */
	unsigned char b_count;		/* users using this block */
	unsigned char b_lock;		/* 0 - ok, 1 -locked */
	struct task_struct * b_wait;
	struct buffer_head * b_prev;
	struct buffer_head * b_next;
	struct buffer_head * b_prev_free;
	struct buffer_head * b_next_free;
};

struct d_inode {
	unsigned short i_mode;
	unsigned short i_uid;
	unsigned long i_size;
	unsigned long i_time;
	unsigned char i_gid;
	unsigned char i_nlinks;
	unsigned short i_zone[9];
};

struct m_inode {
	unsigned short i_mode;
	unsigned short i_uid;
	unsigned long i_size;
	unsigned long i_mtime;
	unsigned char i_gid;
	unsigned char i_nlinks;
	unsigned short i_zone[9];
/* these are in memory also */
//tpc       		struct task_struct * i_wait;
	unsigned long i_atime;
	unsigned long i_ctime;
	unsigned short i_dev;
	unsigned short i_num;
	unsigned short i_count;
	unsigned char i_lock;
	unsigned char i_dirt;
	unsigned char i_pipe;
	unsigned char i_mount;
	unsigned char i_seek;
	unsigned char i_update;
};

struct file {
	unsigned short f_mode;
	unsigned short f_flags;
	unsigned short f_count;
	struct m_inode * f_inode;
//tpc       	off_t f_pos;
};

struct super_block {
	unsigned short s_ninodes;
	unsigned short s_nzones;
	unsigned short s_imap_blocks;
	unsigned short s_zmap_blocks;
	unsigned short s_firstdatazone;
	unsigned short s_log_zone_size;
	unsigned long s_max_size;
	unsigned short s_magic;
/* These are only in memory */
	struct buffer_head * s_imap[8];
	struct buffer_head * s_zmap[8];
	unsigned short s_dev;
	struct m_inode * s_isup;
	struct m_inode * s_imount;
	unsigned long s_time;
//tpc       		struct task_struct * s_wait;
	unsigned char s_lock;
	unsigned char s_rd_only;
	unsigned char s_dirt;
};

struct d_super_block {
	unsigned short s_ninodes;
	unsigned short s_nzones;
	unsigned short s_imap_blocks;
	unsigned short s_zmap_blocks;
	unsigned short s_firstdatazone;
	unsigned short s_log_zone_size;
	unsigned long s_max_size;
	unsigned short s_magic;
};

struct dir_entry {
	unsigned short inode;
	char name[NAME_LEN];
};

extern struct m_inode inode_list[NR_INODE];
extern struct file file_table[NR_FILE];
extern struct super_block super_block_list[NR_SUPER];
extern struct buffer_head * start_buffer;
extern int nr_buffers;

#define S_IFMT  00170000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000
#define S_ISUID  0004000
#define S_ISGID  0002000
#define S_ISVTX  0001000

#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)


/*#################blk##################*/
/*Buffer.c*/
extern void wait_on_buff(struct buffer_head * bh);
extern void lock_buff(struct buffer_head * bh);
extern void unlock_buff(struct buffer_head * bh);
extern void sync_dev(int dev);
extern void invalidate_buff(unsigned int dev, unsigned int block);
extern void invalidate_buffs(int dev);
extern struct buffer_head * retain_buff(unsigned int dev, unsigned int block);
extern struct buffer_head * get_buff(unsigned int dev, unsigned int block);
extern void put_buff(struct buffer_head * buf);
extern struct buffer_head * read_buff(unsigned int dev, unsigned int block);
extern void read_buff_page(unsigned long addr, unsigned int dev, unsigned int b[4]);
extern struct buffer_head * read_buff_ahead(unsigned int dev, unsigned int block,...);
extern void init_buff(long buffer_end);

/*blk_main.c*/
extern int init_hd_info(unsigned int drives_addr, unsigned int drives_length);
extern void ll_rw_block(int rw, struct buffer_head * bh);
extern void init_blk(multiboot_info_t * mbi);

/*#################fs##################*/
/*super.c*/
extern void lock_super(struct super_block * sb);
extern void unlock_super(struct super_block * sb);
extern void wait_on_super(struct super_block * sb);
extern struct super_block * find_super(unsigned int dev);
extern struct super_block * read_super(unsigned int dev);
extern void put_super(int dev);
extern int add_inode_bitmap(unsigned int dev);
extern void free_inode_bitmap(unsigned int dev, unsigned int inode_num);
extern int add_zone_bitmap(unsigned int dev);
extern void free_zone_bitmap(unsigned int dev, unsigned int block);

/*inode.c*/
extern void lock_inode(struct m_inode * inode);
extern void unlock_inode(struct m_inode * inode);
extern void wait_on_inode(struct m_inode * inode);
extern void sync_all_inodes_to_buff(void);
extern struct m_inode * find_inode(unsigned int * dev, unsigned int * nr);
extern struct m_inode * retain_inode(unsigned int * dev, unsigned int * nr);
extern struct m_inode * get_inode(unsigned int dev, unsigned int nr);
extern void put_inode(struct m_inode * inode);
extern struct m_inode * read_inode(int dev,int nr);
extern void delete_inode(struct m_inode * inode);
extern struct m_inode * create_inode(unsigned int dev);
extern int locate_by_inode(struct m_inode * inode,int block,int create);
extern void truncate(struct m_inode * inode);
extern void test_inode(void);

/*zone.c*/
extern unsigned int create_block(unsigned int dev);
extern void delete_block(unsigned int dev, unsigned int block);
extern void delete_indirecte_block(int dev,int block);
extern void delete_double_indirecte_block(int dev,int block);

/*sys_fs.c*/
extern void sys_setup(multiboot_info_t * mbi);

extern struct m_inode * namei(const char * pathname);
extern int open_namei(const char * pathname, int flag, int mode,
	struct m_inode ** res_inode);



extern int ROOT_DEV;

extern void mount_root(void);

#endif
