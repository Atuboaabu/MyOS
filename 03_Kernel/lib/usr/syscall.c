#include "stdint.h"
#include "syscall.h"
#include "print.h"
#include "console.h"
#include "string.h"
#include "fs.h"

/***** 系统调用内核处理函数表 *****/
void* g_Syscall_Table[SYSCALL_MAX];
/* 系统调用内核处理函数表初始化 */
extern uint32_t sys_getpid();
extern void* sys_malloc(uint32_t size);
extern void sys_free(void* ptr);
extern int32_t cls_screen(void);

void syscall_init() {
    put_str("syscall_init start\n");
    g_Syscall_Table[SYS_GETPID] = sys_getpid;
    g_Syscall_Table[SYS_WRITE]  = sys_write;
    g_Syscall_Table[SYS_READ]  = sys_read;
    g_Syscall_Table[SYS_MALLOC]  = sys_malloc;
    g_Syscall_Table[SYS_FREE]  = sys_free;
    g_Syscall_Table[SYS_PUTCHAR]  = sys_putchar;
    g_Syscall_Table[SYS_CLEAR]  = cls_screen;
    put_str("syscall_init done\n");
}

/***** 不同参数的系统调用宏 *****/
/* 无参数的系统调用 */
#define _syscall0(NUMBER) ({        \
    int retval;                     \
    asm volatile (                  \
    "int $0x80"                     \
    : "=a" (retval)                 \
    : "a" (NUMBER)                  \
    : "memory"                      \
    );                              \
    retval;                         \
})

/* 一个参数的系统调用 */
#define _syscall1(NUMBER, ARG1) ({      \
    int retval;                         \
    asm volatile (                      \
    "int $0x80"                         \
    : "=a" (retval)                     \
    : "a" (NUMBER), "b" (ARG1)          \
    : "memory"                          \
    );                                  \
    retval;                             \
})

/* 两个参数的系统调用 */
#define _syscall2(NUMBER, ARG1, ARG2) ({        \
    int retval;                                 \
    asm volatile (                              \
    "int $0x80"                                 \
    : "=a" (retval)                             \
    : "a" (NUMBER), "b" (ARG1), "c" (ARG2)      \
    : "memory"                                  \
    );                                          \
    retval;                                     \
})

/* 三个参数的系统调用 */
#define _syscall3(NUMBER, ARG1, ARG2, ARG3) ({              \
    int retval;                                             \
    asm volatile (                                          \
    "int $0x80"                                             \
    : "=a" (retval)                                         \
    : "a" (NUMBER), "b" (ARG1), "c" (ARG2), "d" (ARG3)      \
    : "memory"                                              \
    );                                                      \
    retval;                                                 \
})

/***** 系统调用函数 *****/
uint32_t getpid() {
    return _syscall0(SYS_GETPID);
}

uint32_t write(int32_t fd, const void* buf, uint32_t size) {
   return _syscall3(SYS_WRITE, fd, buf, size);
}

/* 从文件描述符 fd 中读取 size 个字节到 buf */
int32_t read(int32_t fd, void* buf, uint32_t size) {
    return _syscall3(SYS_READ, fd, buf, size);
}

/* 输出一个字符 */
int32_t putchar(char c) {
    return _syscall1(SYS_PUTCHAR, c);
}

/* 清屏 */
int32_t clear() {
    return _syscall0(SYS_CLEAR);
}

/* 申请 size 字节大小的内存 */
void* malloc(uint32_t size) {
    return (void*)_syscall1(SYS_MALLOC, size);
}

/* 释放 ptr 指向的内存 */
void free(void* ptr) {
    _syscall1(SYS_FREE, ptr);
}