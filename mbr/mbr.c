void enable_pm();
void jump_kernel();

static void outb_p(unsigned char value, unsigned short port) {
  __asm__ __volatile__("outb %%al, %%dx" ::"a"(value), "d"(port) :);
}

static unsigned char inb(unsigned short port) {
  unsigned long eax;
  __asm__ __volatile__("inb %%dx, %%al" : "=a"(eax) : "d"(port) :);
  return (unsigned char)(eax & 0xff);
}

typedef struct {
  unsigned short length;
  unsigned long addr;
} __attribute__((packed)) gdt_ptr_t;

__attribute__((aligned(0x8))) unsigned long long gdt[3] = {
    0,
    0x00CF9A0000000000, // code
    0x00CF920000000000, // data
};

__attribute__((aligned(0x8))) gdt_ptr_t gdt_ptr = {
    .length = sizeof(gdt) - 1,
    .addr = (unsigned long)(gdt),
};

int main() {
  enable_pm();

  __asm__ __volatile__
      (
        "jmp %0, %1\n" 
        :: "i"(1<<3), "i"(jump_kernel)
      );
  return 0;
}

__attribute__((noinline)) void enable_pm() {
  unsigned long cr0;
  // enable a20
  if (!(inb(0x92) & 2)) {
    outb_p(inb(0x92) | 2, 0x92);
  }

  // load gdt
  __asm__ __volatile__("lgdt %0" ::"m"(gdt_ptr) :);

  // 开启 cr0 保护位
  __asm__ __volatile__("movl %%cr0, %0" : "=r"(cr0)::);
  cr0 |= 1;
  __asm__ __volatile__("movl %0, %%cr0" ::"r"(cr0) :);
}
