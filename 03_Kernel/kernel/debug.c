#include "debug.h"
#include "print.h"
#include "interrupt.h"

void panic_spain(const char* file, const int line, const char* func, const char* condition)
{
    interrupt_disable();  // 关闭中断

    put_str("Assert Error!!!\n");

    put_str("file: ");
    put_str(file);
    put_char('\n');

    put_str("line: ");
    put_int(line);
    put_char('\n');

    put_str("func: ");
    put_str(func);
    put_char('\n');

    put_str("condition: ");
    put_str(condition);
    put_char('\n');

    while(1);
}
