#ifndef _KERNEL_DEBUG_H_
#define _KERNEL_DEBUG_H_

/* 断言处理函数 */
void panic_spain(const char* file, const int line, const char* func, const char* condition);

/* 断言处理宏 */
#define PANIC(...) panic_spain(__FILE__, __LINE__, __func__, __VA_ARGS__)

/* 断言：条件不满足进入断言 */
#define ASSERT(condition)   \
    if (!condition) {       \
        PANIC(#condition);  \
    }                       \

#endif