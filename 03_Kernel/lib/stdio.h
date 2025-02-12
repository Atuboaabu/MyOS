#ifndef _STDIO_H_
#define _STDIO_H_

#include "stdint.h"

typedef char* va_list;
/* 把 ap 指向第一个固定参数v */
#define va_start(ap, v) (ap = (va_list)&v)
/* ap 指向下一个参数并返回其值 */
#define va_arg(ap, t) (*((t*)(ap += 4)))
/* 清除 ap */
#define va_end(ap) (ap = NULL)

uint32_t printf(const char* format, ...);
uint32_t vsprintf(char* str, const char* format, va_list ap);
uint32_t sprintf(char* buf, const char* format, ...);

#endif
