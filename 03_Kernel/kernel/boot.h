#ifndef _KERNRL_BOOT_H_
#define _KERNRL_BOOT_H_

#include "stdint.h"

/* GDT结构定义 */
struct GDT_DESC {
    uint16_t limit_0_15;      // 段界限 0~15 位
    uint16_t base_0_15;       // 段基址 0~15 位
    uint8_t  base_16_23;      // 段基址 16~23 位
    uint8_t  type : 4;        // 段类型: 4 bit
    uint8_t  s : 1;           // 系统标记: 1 bit，0为系统段，1为非系统段
    uint8_t  dpl : 2;         // 描述符特权级: 2 bit
    uint8_t  p : 1;           // 是否存在标记: 1 bit
    uint8_t limit_16_19 : 4;  // 段界限 16~19 位
    uint8_t avl : 1;          // 可用标记: 1 bit
    uint8_t l : 1;            // 是否64位代码段标记: 1 bit
    uint8_t d_b : 1;          // 16/32位操作数/代码段标记: 1 bit
    uint8_t g : 1;            // 粒度标记: 1 bit
    uint8_t base_24_31;       // 段基址 24~31 位
};

/* 选择子部分宏定义 */
#define SL_RPL0   (0)
#define SL_RPL1   (1)
#define SL_RPL2   (2)
#define SL_RPL3   (3)

#define SL_TI_GDT (0)
#define SL_TI_LDT (1)

#define SL_CODE  ((0x0001 << 3) + SL_TI_GDT + SL_RPL0)  // 代码段选择子
#define SL_DATA  ((0x0002 << 3) + SL_TI_GDT + SL_RPL0)  //数据段选择子
#define SL_VIDEO ((0x0003 << 3) + SL_TI_GDT + SL_RPL0)  //显存段选择子
#define SL_TSS   ((0x0004 << 3) + SL_TI_GDT + SL_RPL0)  //显存段选择子

/* IDT 描述符部分宏定义 */
#define IDT_DESC_P1        (1 << 7)
#define IDT_DESC_P0        (0 << 7)
#define IDT_DESC_DPL0      (0 << 5)
#define IDT_DESC_DPL3      (3 << 5)
#define IDT_DESC_TYPE_32   (0xE)
#define IDT_DESC_TYPE_16   (0x6)
#define IDT_DESC_ATTR_DPL0 ((IDT_DESC_P1) + (IDT_DESC_DPL0) + (IDT_DESC_TYPE_32))
#define IDT_DESC_ATTR_DPL3 ((IDT_DESC_P1) + (IDT_DESC_DPL3) + (IDT_DESC_TYPE_32))

#endif