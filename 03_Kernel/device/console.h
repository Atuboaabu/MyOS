#ifndef _DEVICE_CONSOLE_H_
#define _DEVICE_CONSOLE_H_

#include "stdint.h"

/* 终端打印字符串 */
void console_put_str(const uint8_t* str);
/* 终端打印字符 */
void console_put_char(const uint8_t c);
/* 终端打印整数 */
void console_put_int(const uint32_t num);

void console_init();

#endif
