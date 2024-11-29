#ifndef _KERNEL_INTERRUPT_H_
#define _KERNEL_INTERRUPT_H_

#include "stdint.h"

/*  函数接口定义 */
void idt_init();

/* IDT 相关定义 */
#define IDT_SIZE (0x21)

struct gate_desc {
    uint16_t text_offset_low;
    uint16_t selector;
    uint8_t  dcount;
    uint8_t  attribute;
    uint16_t text_offset_high;
};

/* 可编程中断控制器8259A相关定义 */
#define PIC_M_CTRL (0x20)  // 8259A 主片的控制端口是0x20
#define PIC_M_DATA (0x21)  // 8259A 主片的数据端口是0x21
#define PIC_S_CTRL (0xA0)  // 8259A 从片的控制端口是0xA0
#define PIC_S_DATA (0xA1)  // 8259A 从片的数据端口是0xA1

#endif