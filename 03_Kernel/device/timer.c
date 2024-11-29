#include "io.h"
#include "timer.h"
#include "print.h"

void timer_set(uint8_t port, uint8_t sc, uint8_t rw,
               uint8_t mode, uint8_t bcd, uint16_t cnt_value)
{
    /* 1、初始化计时器：写入控制字寄存器 */
    uint8_t ctl_val = (uint8_t)(sc << 6 | rw << 4 | mode << 1 | bcd);
    outb(PIT_CTLPORT_CTL, ctl_val);

    /* 2、写入计数器端口：先写入低8位，再写入高8位 */
    uint8_t cnt_val_low = (uint8_t)cnt_value;
    uint8_t cnt_val_high = (uint8_t)(cnt_value >> 8);
    outb(port, cnt_val_low);
    outb(port, cnt_val_high);
}

void timer_init()
{
    put_str("timer init start!\n");
    timer_set(PIT_CTLPORT_CNT0, PIT_SC_00, PIT_RW_11, PIT_MODE_010, PIT_BCD_0, FREQUENCY_SET(100));
    put_str("timer init done!\n");
}