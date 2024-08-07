#ifndef _ASM_H
#define _ASM_H

static void outb_p(unsigned char value, unsigned short port) {
  __asm__ __volatile__(
          "outb %%al, %%dx\n" 
          "jmp 1f\n" 
          "1:\n" 
          "jmp 2f\n" 
          "2:\n" 
          ::"a"(value), "d"(port) :);
}

static unsigned char inb(unsigned short port) {
  unsigned long eax;
  __asm__ __volatile__(
          "inb %%dx, %%al\n" 
          "jmp 1f\n" 
          "1:\n" 
          "jmp 2f\n" 
          "2:\n" 
          : "=a"(eax) : "d"(port) :);
  return (unsigned char)(eax & 0xff);
}

#endif
