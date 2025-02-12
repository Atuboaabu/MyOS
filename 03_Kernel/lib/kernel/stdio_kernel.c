#include "stdio_kernel.h"
#include "stdio.h"
#include "console.h"

void printk(const char* format, ...) {
    va_list args;
    va_start(args, format);  // 使args指向format
    char buf[1024] = { 0 };  // 用于存储拼接后的字符串
    vsprintf(buf, format, args);
    va_end(args);
    console_put_str(buf);
}
