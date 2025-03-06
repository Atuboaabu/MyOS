#include "stdint.h"
#include "syscall.h"
#include "print.h"
#include "console.h"
#include "string.h"
#include "fs.h"
#include "fork.h"
#include "exec.h"
#include "system.h"
#include "thread.h"

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
    g_Syscall_Table[SYS_GETCWD]  = sys_getcwd;
    g_Syscall_Table[SYS_MKDIR]  = sys_mkdir;
    g_Syscall_Table[SYS_RMDIR]  = sys_rmdir;
    g_Syscall_Table[SYS_STAT]  = sys_stat;
    g_Syscall_Table[SYS_OPENDIR]  = sys_opendir;
    g_Syscall_Table[SYS_READDIR]  = sys_readdir;
    g_Syscall_Table[SYS_REWINDDIR]  = sys_rewinddir;
    g_Syscall_Table[SYS_CLOSEDIR]  = sys_closedir;
    g_Syscall_Table[SYS_CHDIR]  = sys_chdir;
    g_Syscall_Table[SYS_FORK]  = sys_fork;
    g_Syscall_Table[SYS_EXECV]  = sys_execv;
    g_Syscall_Table[SYS_EXIT]  = sys_exit;
    g_Syscall_Table[SYS_WAIT]  = sys_wait;
    g_Syscall_Table[SYS_PS]    = sys_ps;
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

/* 获取当前工作目录 */
char* getcwd() {
    return _syscall0(SYS_GETCWD);
}

/* 创建目录 */
int32_t mkdir(const char* pathname) {
    return _syscall1(SYS_MKDIR, pathname);
}

/* 删除空目录: 成功时返回0, 失败时返回-1*/
int32_t rmdir(const char* pathname) {
    return _syscall1(SYS_RMDIR, pathname);
}

/* 获取文件属性信息 */
int32_t stat(const char* pathname, struct stat_info* buf) {
    return _syscall2(SYS_STAT, pathname, buf);
}

/* 打开目录：成功后返回目录指针, 失败返回 NULL */
struct dir* opendir(const char* name) {
    return _syscall1(SYS_OPENDIR, name);
}

/* 当前工作目录为绝对路径path: 成功则返回0, 失败返回-1 */
int32_t chdir(const char* path) {
    return _syscall1(SYS_CHDIR, path);
}

/* 读取目录 dir 的1个目录项: 成功后返回其目录项地址, 到目录尾时或出错时返回 NULL */
struct dir_entry* readdir(struct dir* dir) {
    return _syscall1(SYS_READDIR, dir);
}

/* 把目录dir的指针dir_pos置0 */
void rewinddir(struct dir* dir) {
    _syscall1(SYS_REWINDDIR, dir);
}

/* 关闭目录 dir: 成功返回 0, 失败返回-1 */
int32_t closedir(struct dir* dir) {
    return _syscall1(SYS_CLOSEDIR, dir);
}

/* fork子进程：成功后父进程返回子进程 pid，子进程返回0；失败返回 -1 */
int32_t fork() {
    return _syscall0(SYS_FORK);
}

/* 让进程执行path路径对应的文件，argv为指定的参数 */
int32_t execv(const char* path, const char* argv[]) {
    return _syscall2(SYS_EXECV, path, argv);
}

/* 进程用来结束调用 */
void exit(int32_t status) {
    return _syscall1(SYS_EXIT, status);
}

/* 等待子进程调用 exit, 将子进程的退出状态保存到status指向的变量: 成功则返回子进程的pid, 失败则返回 -1 */
int32_t wait(int32_t* status) {
    return _syscall1(SYS_WAIT, status);
}

/* 显示进程状态 */
void ps() {
    _syscall0(SYS_PS);
}