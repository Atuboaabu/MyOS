#ifndef _DEVICE_PIT8253_H_
#define _DEVICE_PIT8253_H_

/* 函数接口定义部分 */
void timer_init();

/* PIT 8253控制字相关定义 */
#define PIT_SC_00    (0)  // Select Chanel 0
#define PIT_SC_01    (1)  // Select Chanel 1
#define PIT_SC_10    (2)  // Select Chanel 2
#define PIT_RW_00    (0)  // 锁存数据，供CPU读
#define PIT_RW_01    (1)  // 只读写低字节
#define PIT_RW_10    (2)  // 只读写高字节
#define PIT_RW_11    (3)  // 先读写低字节，后读写高字节
#define PIT_MODE_000 (0)  // 中断生成器
#define PIT_MODE_001 (1)  // 硬件可编程单脉冲生成器
#define PIT_MODE_010 (2)  // 定时器模式（分频模式）
#define PIT_MODE_011 (3)  // 方波生成模式
#define PIT_MODE_100 (4)  // 软件触发定时器
#define PIT_MODE_101 (5)  // 硬件触发定时器
#define PIT_BCD_0    (0)  // 二进制模式
#define PIT_BCD_1    (1)  // BCD模式

/* 控制字端口相关定义 */
#define PIT_CTLPORT_CNT0 (0x40)  // 计数器0端口号
#define PIT_CTLPORT_CNT1 (0x41)  // 计数器1端口号
#define PIT_CTLPORT_CNT2 (0x42)  // 计数器2端口号
#define PIT_CTLPORT_CTL  (0x43)  // 控制寄存器端口号

#define PIT_CNT0_FREQUENCY (1193180)  // 计数器0工作脉冲频率
#define FREQUENCY_SET(freq) (PIT_CNT0_FREQUENCY / (freq))  // 设置工作评率

#endif