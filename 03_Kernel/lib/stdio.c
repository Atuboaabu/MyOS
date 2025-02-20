#include "stdio.h"
#include "stdint.h"
#include "string.h"
#include "syscall.h"

/* 将整型转换成字符 */
static void itoa(uint32_t value, char** buf_ptr, uint8_t base) {
    uint32_t m = value % base;
    uint32_t i = value / base;
    if (i) {  // i != 0，递归
        itoa(i, buf_ptr, base);
    }
    if (m < 10) {  // 余数 < 10 转换为 '0'~'9'
        *((*buf_ptr)++) = m + '0';
    } else {  // 余数 >= 10 转换为 'A'~'F'
        *((*buf_ptr)++) = m - 10 + 'A';
    }
}

/* 将参数 ap 按照格式 format 输出到字符串str, 并返回替换后str长度 */
uint32_t vsprintf(char* str, const char* format, va_list ap) {
    char* buf_ptr = str;
    const char* format_ptr = format;
    char format_char = *format_ptr;
    char* arg_str;
    int32_t arg_int;
    while(format_char) {
        if (format_char != '%') {  // 字符不为 %，无格式化操作
            *(buf_ptr++) = format_char;
            format_char = *(++format_ptr);
            continue;
        }
        format_char = *(++format_ptr);  // 获取 % 后的字符，判断格式化类型
        switch(format_char) {
            case 's':
                arg_str = va_arg(ap, char*);
                strcpy(buf_ptr, arg_str);
                buf_ptr += strlen(arg_str);
                format_char = *(++format_ptr);
                break;
            case 'c':
                *(buf_ptr++) = va_arg(ap, char);
                format_char = *(++format_ptr);
                break;
            case 'd':
                arg_int = va_arg(ap, int);
                /* 若是负数, 将其转为正数后,再正数前面输出个负号'-'. */
                if (arg_int < 0) {
                    arg_int = 0 - arg_int;
                    *buf_ptr++ = '-';
                }
                itoa(arg_int, &buf_ptr, 10); 
                format_char = *(++format_ptr);
                break;
            case 'x':
                arg_int = va_arg(ap, int);
                itoa(arg_int, &buf_ptr, 16); 
                format_char = *(++format_ptr);
                break;
        }
    }
    return strlen(str);
}

uint32_t sprintf(char* buf, const char* format, ...) {
    va_list args;
    va_start(args, format);
    uint32_t retval = vsprintf(buf, format, args);
    va_end(args);
    return retval;
}

uint32_t printf(const char* format, ...) {
    va_list args;
    va_start(args, format);  // 使args指向format
    char buf[1024] = { 0 };  // 用于存储拼接后的字符串
    vsprintf(buf, format, args);
    va_end(args);
    return write(1, buf, strlen(buf));
}