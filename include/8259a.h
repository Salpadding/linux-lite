#ifndef _8259A_H
#define _8259A_H

#include <asm.h>

// 8259a = 一主一丛各自8个 IR, 4个ICW，三个OCW

// 8259a 分主和从 各自有 7 个 IRQ(IRQ0~IRQ7)
// 从芯片的 IRQ1 级连到主芯片的 IRQ2
// 8259A寄存器 = ICW*4 + OCW*3
// ICW用于初始化 8259A
#define _8259A_M_CMD 0x20  // 主片控制端口
#define _8259A_M_DATA 0x21 // 主片数据端口
#define _8259A_S_CMD 0xA0  // 从片控制端口
#define _8259A_S_DATA 0xA1 // 从片数据端口
#define _8259A_M_IRQ_OFF 0x20
#define _8259A_S_IRQ_OFF 0x28
#define _8259A_VEC_CLOCK (_8259A_M_IRQ_OFF + 0) // 时钟中断 的中断向量号
#define _8259A_VEC_SATA0 (_8259A_S_IRQ_OFF + 6) // sata0 的中断向量号
#define _8259A_VEC_SATA1 (_8259A_S_IRQ_OFF + 7) // sata1 的中断向量号

#define _8259A_IRQ_TIMER 0
#define _8259A_IRQ_KEYBOARD 1
#define _8259A_IRQ_CASCADE 2
#define _8259A_IRQ_COM2_COM4 3
#define _8259A_IRQ_COM1_COM3 4
#define _8259A_IRQ_IDE_PRIMARY 14
#define _8259A_IRQ_IDE_SECONDARY 15


void _8259A_init(int ocw, char ocw_m, char ocw_s);

void _8259a_unmask(unsigned short bit);

static void _8259A_EOI() {
  outb_p(0x20, _8259A_S_CMD);
  outb_p(0x20, _8259A_M_CMD);
}

#endif
