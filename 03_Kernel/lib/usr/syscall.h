#ifndef _LIB_SYSCALL_H_
#define _LIB_SYSCALL_H_

/* 系统调用类型枚举 */
enum syscall_nr {
    SYS_GETPID,
    SYS_WRITE,
    SYS_READ,
    SYS_MALLOC,
    SYS_FREE,
    SYS_PUTCHAR,
    SYS_CLEAR,
    SYSCALL_MAX
};

/* 系统调用的接口 */
uint32_t getpid();
uint32_t write(int32_t fd, const void* buf, uint32_t size);
int32_t read(int32_t fd, void* buf, uint32_t size);
void* malloc(uint32_t size);
void free(void* ptr);
int32_t putchar(char c);
int32_t clear();

#endif
