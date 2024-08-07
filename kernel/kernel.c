#include <buffer.h>
#include <kernel.h>
#include <memory.h>
#include <print.h>
#include <sched.h>
#include <string.h>
#include <sys_call.h>
#include <trap.h>
#include <types.h>

#define move_to_user_mode(esp)                                                 \
  __asm__ __volatile__("lea %0,%%eax\n\t"                                      \
                       "pushl $0x17\n\t"                                       \
                       "pushl %%eax\n\t"                                       \
                       "pushfl\n\t"                                            \
                       "pushl $0x0f\n\t"                                       \
                       "pushl $1f\n\t"                                         \
                       "iret\n"                                                \
                       "1:\tmovl $0x17,%%eax\n\t"                              \
                       "1:\tmovl %%esp,%%ebp\n\t"                              \
                       "mov %%ax,%%ds\n\t"                                     \
                       "mov %%ax,%%es\n\t"                                     \
                       "mov %%ax,%%fs\n\t"                                     \
                       "mov %%ax,%%gs" ::"m"(esp)                              \
                       : "ax")

__attribute__((aligned(8))) unsigned long long idt[256];

__attribute__((aligned(8))) desc_ptr_t idt_ptr = {
    .length = sizeof(idt) - 1,
    .addr = idt,
};

__attribute__((aligned(8))) unsigned long long gdt[256];

__attribute__((aligned(8))) desc_ptr_t gdt_ptr = {
    .length = sizeof(gdt) - 1,
    .addr = gdt,
};

static void enable_sse() {
  unsigned int cr4;

  __asm__ __volatile__("mov %%cr4, %0\n" : "=r"(cr4));

  // Set OSFXSR (bit 9) and OSXMMEXCPT (bit 10)
  cr4 |= (1 << 9) | (1 << 10);

  __asm__ __volatile__("mov %0, %%cr4\n" : : "r"(cr4));
}

__attribute__((aligned(PAGE_SIZE))) char init_user_stack[PAGE_SIZE];

void init();

int main() {
  // setup fpu, write protection
  __asm__ __volatile__("mov %%cr0, %%eax\n"
                       "or  $0x10002, %%eax\n"
                       "mov %%eax, %%cr0\n"
                       "fninit\n"
                       :
                       :
                       : "eax");

  memset(&_bss, 0, ((unsigned long)&_ebss) - ((unsigned long)&_bss));

  // init gdt
  gdt[1] = 0x00CF9A000000ffff;
  gdt[2] = 0x00CF92000000ffff;

  __asm__ __volatile__("lidt %0\n"
                       "lgdt %1\n" ::"m"(idt_ptr),
                       "m"(gdt_ptr));

  enable_sse();
  print_init();
  mem_init();
  buffer_init();
  trap_init();
  sched_init();
  sys_call_table_init();

  printk("kernel init down\n");

  __asm__ __volatile__("sti\n");

  // enter use mode
  move_to_user_mode((init_user_stack[PAGE_SIZE]));

  // 从这里开始不能 printk 了 因为 printk 是往端口写东西 要用 write 系统调用
  if (!fork()) {
    init();
  }

  while (1) {
  }
}

void init() {
  setup();
  while (1)
    ;
}
