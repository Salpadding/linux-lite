#ifndef _BUFFER_H
#define _BUFFER_H

#ifndef BUFFER_MEMORY_END
#define BUFFER_MEMORY_END (1 << 23)
#endif

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

void buffer_init();
void brelse(struct buffer_head *buf);
struct buffer_head *bread(int dev, int block);

extern int NR_BUFFERS;
extern struct buffer_head * buffer_free_list;
#endif
