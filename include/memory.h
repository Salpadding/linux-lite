#ifndef _MEMORY_H
#define _MEMORY_H
#include <kernel.h>

#ifndef PAGING_MEMORY
#define PAGING_MEMORY (64 * 1024 * 1024)
#endif

#define HIGH_MEMORY PAGING_MEMORY

#define PAGING_PAGES (PAGING_MEMORY >> 12)

// 内核保留使用的内存 (40MB) 需要建立恒等映射
#define PAGING_RESERVED_SIZE (40 << 20)

// 通常来说 dirty bit 被 mmu 自动设置
#define PAGE_DIRTY	0x40
#define PAGE_ACCESSED	0x20
#define PAGE_USER	0x04
#define PAGE_RW		0x02
#define PAGE_PRESENT	0x01

extern unsigned char mem_map[PAGING_PAGES];

#define MAP_NR(addr) (((unsigned long)(addr)) >> 12)

void mem_init();

typedef unsigned long page_table[PAGE_SIZE >> 2];

// 页目录表 丢到 bss 去
extern page_table page_dir;

unsigned long get_free_page(void);

void free_page(unsigned long addr);

int copy_page_tables(unsigned long from,unsigned long to,long size);

int free_page_tables(unsigned long from,unsigned long size);

int copy_init(unsigned long to);

int mmap(unsigned long vstart, unsigned long pstart, unsigned long page_nr,
         unsigned long bits);


void do_wp_page(unsigned long error_code,unsigned long address);
void un_wp_page(unsigned long * table_entry);
#endif
