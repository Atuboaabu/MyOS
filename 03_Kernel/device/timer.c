#include "io.h"
#include "timer.h"
#include "print.h"
#include "interrupt.h"
#include "thread.h"

/* 时钟滴答频率 */
#define IRQ0_FREQUENCY (100)
/* 每次中断 ms 数 */
#define MIL_SECONDS_PER_INTERUPT (1000 / IRQ0_FREQUENCY)
/* 系统启动后时钟的所有滴答数 */
uint32_t g_tickCount;

static void timer_inter_handle() {
    struct PCB_INFO* curThreadPCB = get_curthread_pcb();
    g_tickCount++;
    if (curThreadPCB == NULL) {
        put_str("curThreadPCB is NULL!");
        put_char("\n");
        return;
    }
    if (curThreadPCB->stack_magic != 0x19870916) {
        put_str("curThreadPCB stack overflow!");
        put_char("\n");
        return;
    }
    if (curThreadPCB->ticks == 0) {
        thread_schedule();
    } else {
        curThreadPCB->ticks--;
        // put_str("\n ticks = ");
        // put_int(curThreadPCB->ticks);
        // put_char('\n');
    }
    curThreadPCB->elapsed_ticks++;
}

/* 以 tick 为单位的sleep, 任何时间形式的 sleep 会转换此ticks形式 */
static void tick_sleep(uint32_t ticks) {
    uint32_t start_tick = g_tickCount;

    while (g_tickCount - start_tick < ticks) {	   // 若间隔的ticks数不够便让出cpu
        thread_yield();
    }
}

/* ms级别的休眠函数 */
void m_sleep(uint32_t ms) {
    uint32_t sleep_ticks = (ms + MIL_SECONDS_PER_INTERUPT) / MIL_SECONDS_PER_INTERUPT;
    if(sleep_ticks <= 0) {
        return;
    };
    tick_sleep(sleep_ticks); 
}

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
    register_interrupt_handle(0x20, timer_inter_handle);
    put_str("timer init done!\n");
}