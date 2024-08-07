#include <8259a.h>

#define outb outb_p

static void io_wait(void) { outb(0x80, 0); }

// 1. ICW1 初始化流程控制
// 2. ICW2 中断向量号
// 3. ICW3 主从关系
// 4. ICW4 开启 8086 模式
//
// 1. OCW1 IRQ 掩码
// 2. OCw2 用于发送EOI 以及中断优先级
// 3. OCW3 用于读取控制器状态
void _8259A_init(int ocw, char ocw_m, char ocw_s) {
  // 初始化 ICW1~ICW4 必须按照 ICW1~ICW4 的顺序进行
  // 主片:
  // ICW1 OCW2 OCW3 映射到 0x20 端口
  // ICW2~4 OCW1 映射到 0x21 端口
  // 从片:
  // ICW1 OCW2 OCW3 映射到 0xa0 端口
  // ICW2~4 OCW1 映射到 0xa1 端口

  // ICW1: 负责初始化流程控制
  // 5~7 位 pc 机必须位 0
  // 4 ICW 必须为 1
  // 3 触发模式忽略
  // 2 忽略
  // 1 true=单片 false=级联
  // 0 true=使用 ICW4 false= 不使用 ICW4
  outb(0b00010001, _8259A_M_CMD);
  io_wait();
  outb(0b00010001, _8259A_S_CMD);
  io_wait();

  // ICW2: 负责设置中断向量号
  // 我们设置主片的中断向量从 0x20 开始, 从片从 0x28 开始
  // 主0x20~0x27 从 0x28~0x2f
  outb(_8259A_M_IRQ_OFF, _8259A_M_DATA);
  io_wait();
  outb(_8259A_S_IRQ_OFF, _8259A_S_DATA);
  io_wait();

  // ICW3: 负责设置主从关系
  // 主片 IRQ2 级联到从片的 IRQ1
  outb(1 << 2, _8259A_M_DATA);
  io_wait();
  // tell slave it needs to connect to master
  outb(1 << 1, _8259A_S_DATA);
  io_wait();

  // ICW4:
  // 这里通常不会设置的过于复杂
  // 主片 从片 开启 8086 模式
  // 这样后面中断处理中收到 0x20~0x2f 的中断后要向中断处理器发送 EOI 指令
  // 中断请求优先级按 IRQ0~IRQ7 从高到低排序
  outb(1, _8259A_M_DATA);
  io_wait();
  outb(1, _8259A_S_DATA);
  io_wait();

  if (ocw) {
    // 暂时关闭所有中断
    outb(ocw_s, _8259A_S_DATA);
    io_wait();
    outb(ocw_m, _8259A_M_DATA);
    io_wait();
  }
}

void _8259a_unmask(unsigned short bit) {
    unsigned short i = 1;
    i = i << bit;
    
    unsigned char mask = inb(0x21);
    mask = mask & (~(i));
    outb_p(mask & 0xff, 0x21);
}
