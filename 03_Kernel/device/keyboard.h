#ifndef _DEVICE_KEYBOARD_H_
#define _DEVICE_KEYBOARD_H_

/* 键盘中断的中断号 0x21 */
#define KEYBOARD_INTR_NUM (0x21)
/* 8042芯片中键盘数据读取端口号 */
#define KEYBOARD_BUFFER_PORT (0x60)

void keyboard_init();

#endif
