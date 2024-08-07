#include <trap.h>
#include <8259a.h>

void trap_init() {
  // init 8259a and mask all interrupt 
  _8259A_init(1, 0b11111011, 0b11111111);
}
