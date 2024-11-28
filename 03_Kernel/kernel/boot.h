#ifndef _KERNRL_BOOT_H_
#define _KERNRL_BOOT_H_

/* 选择子部分宏定义 */
#define SL_RPL0   (0)
#define SL_RPL1   (1)
#define SL_RPL2   (2)
#define SL_RPL3   (3)

#define SL_TI_GDT (0)
#define SL_TI_LDT (1)

#define SL_CODE  ((0x0001 << 3) + SL_TI_GDT + SL_RPL0)  // 代码段选择子
#define SL_DATA  ((0x0002 << 3) + SL_TI_GDT + SL_RPL0)  //数据段选择子
#define SL_VIDEO ((0x0003 << 3) + SL_TI_GDT + SL_RPL0) ;显存段选择子

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