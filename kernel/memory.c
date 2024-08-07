#include <memory.h>
#include <print.h>
#include <sched.h>
#include <string.h>

#define USED 100

unsigned char mem_map[PAGING_PAGES];

__attribute__((aligned(PAGE_SIZE))) page_table page_dir;

#define invalidate() __asm__("movl %0,%%cr3" ::"r"(page_dir))

// 8MB 以下: lowmemory + 内核 + 缓冲区 保留
// 8MB-40MB: RAMDISK (32MB) 保留
// 40MB-64MB: 可用于分配给用户进程(20MB)
void mem_init() {

  unsigned long start = 0;
  unsigned long i, j;

  // 保留内核用到的物理页
  for (i = 0; i < MAP_NR(PAGING_RESERVED_SIZE); i++) {
    mem_map[i] = USED;
  }

  // 建立恒等映射 不然容易报页错误
  for (i = 0; i < (PAGING_MEMORY >> 22); i++) {
    unsigned long *pt = (void *)get_free_page();
    page_dir[i] = ((unsigned long)pt) | 7;

    for (j = 0; j < (PAGE_SIZE >> 2); j++) {
      pt[j] = (i << 22) | (j << 12) | 7;
    }
  }

  // 加载页表
  __asm__ __volatile__("movl %0, %%cr3\n" : : "r"((unsigned long)page_dir));

  unsigned long cr0;
  __asm__ __volatile__("movl %%cr0, %0\n" : "=r"(cr0) :);
  cr0 = cr0 | 0x80000000;
  __asm__ __volatile__("movl %0, %%cr0\n" ::"r"(cr0));
}

unsigned long get_free_page(void) // filled it with zero
{
  register unsigned long __res;
repeat:
  __asm__ __volatile__
      // repne, repeat while not equal
      ("std ; repne ; scasb\n\t"
       "jne 1f\n\t"
       "movb $1,1(%%edi)\n\t" // mark the page as used
       "sall $12,%%ecx\n\t"
       "movl %%ecx,%%edx\n\t"
       "movl $1024,%%ecx\n\t"       // set ecx = PAGE_SIZE/4
       "leal 4092(%%edx),%%edi\n\t" // set edi = last 4 byte of page found
       "rep ; stosl\n\t"
       "movl %%edx,%%eax\n"
       "1:\n"
       "cld\n"
       : "=a"(__res)
       : "0"(0), "c"(PAGING_PAGES), "D"(mem_map + PAGING_PAGES - 1)
       : "edx", "memory", "cc");
  if (!__res || __res >= HIGH_MEMORY)
    goto repeat;
  return __res;
}

void free_page(unsigned long addr) {
  if (addr >= HIGH_MEMORY)
    panic("trying to free nonexistent page");
  if (mem_map[MAP_NR(addr)]--) // -1 之前不为0
    return;
  mem_map[addr] = 0; // -1 之前就是0 减完要给他恢复过去
  panic("trying to free free page");
}

int mmap(unsigned long vstart, unsigned long pstart, unsigned long page_nr,
         unsigned long bits) {
  unsigned long *dir = &page_dir[vstart >> 22];
  unsigned long j = (vstart >> 12) & 0x3ff;
  if (!*dir)
    *dir = (get_free_page() | 7);
  unsigned long *pt = (void *)((*dir) & 0xfffff000);

  while (page_nr--) {
    pt[j] = pstart | (bits & 0xfff);
    j++;
    pstart += PAGE_SIZE;
    if (j == 1024) {
      j = 0;
      dir++;
      if (!*dir)
        *dir = (get_free_page() | 7);
      pt = (void *)((*dir) & 0xfffff000);
    }
  }
  return 0;
}

int copy_init(unsigned long to) {
  unsigned long mem_start = (unsigned long)&_text;
  unsigned long mem_end = (unsigned long)&_end;

  unsigned long *from_dir = &page_dir[mem_start >> 22];
  unsigned long *to_dir = &page_dir[to >> 22];
  unsigned long page_nr = (mem_end - mem_start + PAGE_SIZE - 1) / PAGE_SIZE;

  unsigned long j = (mem_start >> 12) & 0x3ff;

  unsigned long *dst_pt = (void *)get_free_page();

  *to_dir = ((unsigned long)dst_pt) | 7;
  unsigned long *src_pt = (void *)((unsigned long)*from_dir & (0xfffff000));

  while (page_nr--) {
    dst_pt[j] = src_pt[j] & (~(1 << 1));

    j++;
    if (j == 1024) {
      j = 0;
      from_dir++;
      to_dir++;
      dst_pt = (void *)get_free_page();
      *to_dir = ((unsigned long)dst_pt) | 7;
      src_pt = (void *)((unsigned long)*from_dir & (0xfffff000));
    }
  }

  // 用户态栈区域
  unsigned long new_user_stack = get_free_page();
  memcpy((char *)new_user_stack, (char *)init_user_stack, 4096);
  mmap(to + ((unsigned long)(init_user_stack)), new_user_stack, 1, 7);

  invalidate();
  return 0;
}

/* copy on write 的实现框架 */
int copy_page_tables(unsigned long from, unsigned long to, long size) {
  unsigned long *from_page_table;
  unsigned long *to_page_table;
  unsigned long this_page;
  unsigned long *from_dir, *to_dir;
  unsigned long new_page;
  unsigned long nr;
  printk("copy pages from 0x%x to 0x%x with size 0x%x\n", from, to, size);
  if ((from & 0x3fffff) || (to & 0x3fffff)) // 22bit=4MB 32=10+10+12 20=12+10-2
                                            // -2相当于乘以4 4=sizeof(long)
    panic("copy_page_tables called with wrong alignment");

  from_dir = &(page_dir[from >> 22]);
  to_dir = &(page_dir[to >> 22]);
  size = ((unsigned)(size + 0x3fffff)) >>
         22; /* unit of size is 4MB = a page directory */

  for (; size-- > 0; from_dir++, to_dir++) {
    if (1 & *to_dir)
      panic("copy_page_tables: already exist");
    if (!(1 & *from_dir))
      continue;
    from_page_table = (unsigned long *)(0xfffff000 & *from_dir);
    if (!(to_page_table =
              (unsigned long *)get_free_page())) // 申请一个新的页框 用来存 pte
      return -1; /* Out of memory, see freeing */

    *to_dir = ((unsigned long)to_page_table) | 7; // 设置页目录
    nr = 1024;

    for (; nr-- > 0; from_page_table++, to_page_table++) {
      this_page = *from_page_table;
      if (!this_page)
        continue;

      // 比较特殊的情况: 旧的进程存在 present = 0 但是不为 0 的页面
      // 目前不清楚这类 page 出现的原因 或许是 swap ? 这种情况不需要写时拷贝
      if (!(1 & this_page)) { // missing present bit
        if (!(new_page =
                  get_free_page())) // 如果缺少 present bit 这里没有采用 copy by
                                    // reference 而是直接 new 了一个 page
          return -1;

        *to_page_table = this_page; // 把旧的页面换给新的进程
        *from_page_table =
            new_page | (PAGE_DIRTY | 7); // 旧进程换到新的页面 但是标记为 dirty
        continue;
      }
      this_page &= ~2;
      *to_page_table =
          this_page; // 新进程和旧进程 map 到同一个物理页框 但是都设置为只读

      *from_page_table = this_page;
      mem_map[MAP_NR(this_page)]++;
    }
  }
  invalidate();
  return 0;
}

int free_page_tables(unsigned long from, unsigned long size) {
  unsigned long *pg_table;
  unsigned long *dir, nr;

  if (from & 0x3fffff)
    panic("free_page_tables called with wrong alignment");

  size = (size + 0x3fffff) >> 22; // 最小单位 4MB
  dir = &(page_dir[from >> 22]);
  for (; size-- > 0; dir++) {
    if (!(1 & *dir)) // check present bit
      continue;
    pg_table = (unsigned long *)(0xfffff000 & *dir);
    for (nr = 0; nr < 1024; nr++) {
      if (*pg_table) {
        if (1 & *pg_table) // check present bit
          free_page(0xfffff000 & *pg_table);
        *pg_table = 0;
      }
      pg_table++;
    }
    free_page(0xfffff000 & *dir);
    *dir = 0;
  }
  invalidate();
  return 0;
}

static void oom() {
  printk("oom\n");
  while (1)
    ;
}

/*
 * This routine handles present pages, when users try to write
 * to a shared page. It is done by copying the page to a new address
 * and decrementing the shared-page counter for the old page.
 *
 * If it's in code space we exit with a segment error.
 */
void do_wp_page(unsigned long error_code, unsigned long address) {
  if (address < TASK_SIZE) {
    printk("\n\rBAD! KERNEL MEMORY WP-ERR!\n\r");
    while (1)
      ;
  }
  if (address - current->start_code > TASK_SIZE) {
    printk("Bad things happen: page error in do_wp_page\n\r");
    while (1)
      ;
  }
  un_wp_page(
      (unsigned long *)(((address >> 10) & 0xffc) +
                        (0xfffff000 &
                         *((unsigned long *)((address >> 20) & 0xffc)))));
}


#define copy_page(from,to) \
__asm__("cld ; rep ; movsl"::"S" (from),"D" (to),"c" (1024):) 

void un_wp_page(unsigned long *table_entry) {
  unsigned long old_page, new_page;

  old_page = 0xfffff000 & *table_entry;
  if (mem_map[MAP_NR(old_page)] == 1) {
    *table_entry |= 2;
    invalidate();
    return;
  }
  if (!(new_page = get_free_page()))
    oom();

  mem_map[MAP_NR(old_page)]--;
  copy_page(old_page, new_page);
  *table_entry = new_page | 7;
  invalidate();
}
