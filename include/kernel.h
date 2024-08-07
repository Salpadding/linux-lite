#ifndef _KERNEL_H
#define _KERNEL_H
#include <types.h>

#define PAGE_SIZE (1 << 12)

extern void _end;
extern void _bss;
extern void _ebss;
extern void _edata;
extern void _text;

typedef struct {
  unsigned short length;
  void *addr;
} __attribute__((packed)) desc_ptr_t;

extern unsigned long long idt[256];
extern unsigned long long gdt[256];

extern desc_ptr_t idt_ptr;
extern desc_ptr_t gdt_ptr;

#define NGROUPS 32 /* Max number of groups per user */
#define RLIM_NLIMITS 6
#define NR_OPEN 20
#define NOGROUP -1

struct desc_struct {
  unsigned long a, b;
};

typedef struct {
  unsigned short limit_low;
  unsigned short base_low;
  unsigned char base_mid;
  unsigned char access;
  unsigned char granularity;
  unsigned char base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
  uint16_t offset_low; // Lower 16 bits of the handler function address
  uint16_t selector;   // Code segment selector in GDT or LDT
  uint8_t reserved;  // Bits 0-2 hold Interrupt Stack Table offset, rest of the
                     // bits should be zero
  uint8_t type_attr; // Type and attributes: type, DPL, P, etc.
  uint16_t offset_high; // Middle 16 bits of the handler function address
} __attribute__((packed)) idt_entry_t;

extern char init_user_stack[PAGE_SIZE];

#define panic(x)                                                               \
  {                                                                            \
    __asm__ __volatile__("cli\n");                                             \
    printk(x);                                                                 \
    while (1)                                                                  \
      ;                                                                        \
  };


#define cli() __asm__ __volatile__("cli\n")
#define sti() __asm__ __volatile__("sti\n")

#endif
