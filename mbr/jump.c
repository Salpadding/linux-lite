void jump_kernel() {
  __asm__ __volatile__
      (
        "jmp %0, %1" :
        : "i"(1<<3), "i"(0x100000)
      );
}
