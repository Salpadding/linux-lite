#ifndef _SYSTEM_H
#define _SYSTEM_H
#include <kernel.h>

#define FIRST_TSS_ENTRY 4
#define FIRST_LDT_ENTRY (FIRST_TSS_ENTRY + 1)

#define _TSS(n) ((((unsigned long)n) << 4) + (FIRST_TSS_ENTRY << 3))
#define _LDT(n) ((((unsigned long)n) << 4) + (FIRST_LDT_ENTRY << 3))

// type = 14: interrupt gate
// type = 15: trap gate
// selector = code dpl 0
static void _set_gate(char *dst, unsigned char type, unsigned char dpl,
                      unsigned long addr) {
  idt_entry_t *_dst = (idt_entry_t *)dst;
  _dst->selector = 0x8;
  _dst->offset_low = (unsigned short)(addr & 0xffff);
  _dst->type_attr = type | 0x80 | (dpl << 5);
  _dst->offset_high = (unsigned short)((addr >> 16) & 0xffff);
}

static void _set_tssldt_desc(char *dst, unsigned long addr,
                             unsigned char access, unsigned char gran,
                             unsigned short limit) {
  gdt_entry_t *_dst = (gdt_entry_t *)dst;
  _dst->limit_low = limit;
  _dst->base_low = (unsigned short)(addr & 0xffff);
  _dst->base_mid = (unsigned char)((addr & 0xff0000) >> 16);
  _dst->access = access;
  _dst->granularity = gran;
  _dst->base_high = (unsigned char)((addr >> 24) & 0xff);
}

#define set_intr_gate(n, addr)                                                 \
  _set_gate((char *)(&idt[n]), 14, 0, (unsigned long)addr)

#define set_trap_gate(n, addr)                                                 \
  _set_gate((char *)(&idt[n]), 15, 0, (unsigned long)addr)

#define set_system_gate(n, addr)                                               \
  _set_gate((char *)(&idt[n]), 15, 3, (unsigned long)addr)

#define set_tss_desc(n, addr)                                                  \
  _set_tssldt_desc(((char *)(n)), (unsigned long)addr, 0x89, 0x40, 104)
#define set_ldt_desc(n, addr)                                                  \
  _set_tssldt_desc(((char *)(n)), (unsigned long)addr, 0x82, 0x00, 32)

static unsigned long _get_base(const char *addr) {
  gdt_entry_t *dst = (void *)addr;
  return ((unsigned long)dst->base_low) |
         (((unsigned long)dst->base_mid) << 16) |
         (((unsigned long)dst->base_high) << 24);
}


static void _set_base(char* addr, unsigned long base) {
    *((unsigned short*)(&addr[2])) = (unsigned short) (base & 0xffff);
    addr[4] = (char) ((base >> 16) & 0xff);
    addr[7] = (char) ((base >> 24) & 0xff);
}

static void _set_limit(char* addr, unsigned long limit) {
    *((unsigned short*)(&addr[2])) = (unsigned short) (limit & 0xffff);
    addr[6] = (addr[6] & 0xf0) | ((char)((limit >> 16) & 0xf));
}

#define get_base(ldt) _get_base(((char *)&(ldt)))

#define get_limit(segment)                                                     \
  ({                                                                           \
    unsigned long __limit;                                                     \
    /* lsll = load segment limit */ __asm__("lsll %1,%0\n\tincl %0"            \
                                            : "=r"(__limit)                    \
                                            : "r"(segment));                   \
    __limit;                                                                   \
  })

#define set_base(ldt,base) _set_base( ((char *)&(ldt)) , (unsigned long)base )
#define set_limit(ldt,limit) _set_limit( ((char *)&(ldt)) ,(unsigned long) ((limit-1)>>12) )

#endif
