#ifndef _LIB_SYSCALL_H_
#define _LIB_SYSCALL_H_

/* 系统调用类型枚举 */
enum syscall_nr {
    SYS_GETPID,
    SYS_WRITE,
    SYSCALL_MAX
};

/* 系统调用的接口 */
uint32_t getpid();
uint32_t write(char* str);


#endif
