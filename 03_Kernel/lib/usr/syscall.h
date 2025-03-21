#ifndef _LIB_SYSCALL_H_
#define _LIB_SYSCALL_H_

#include "fs.h"

/* 系统调用类型枚举 */
enum syscall_nr {
    SYS_GETPID,
    SYS_WRITE,
    SYS_READ,
    SYS_MALLOC,
    SYS_FREE,
    SYS_PUTCHAR,
    SYS_CLEAR,
    SYS_GETCWD,
    SYS_MKDIR,
    SYS_RMDIR,
    SYS_STAT,
    SYS_OPENDIR,
    SYS_READDIR,
    SYS_REWINDDIR,
    SYS_CLOSEDIR,
    SYS_CHDIR,
    SYS_FORK,
    SYS_EXECV,
    SYS_EXIT,
    SYS_WAIT,
    SYS_PS,
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
/* 获取当前工作目录，成功返回路径 buf，失败返回 NULL，返回的 buf 由用户释放 */
char* getcwd();
/* 获取文件属性信息 */
int32_t stat(const char* pathname, struct stat_info* buf);
/* 创建目录 */
int32_t mkdir(const char* pathname);
/* 删除空目录: 成功时返回0, 失败时返回-1*/
int32_t rmdir(const char* pathname);
/* 打开目录：成功后返回目录指针, 失败返回 NULL */
struct dir* opendir(const char* name);
/* 读取目录 dir 的1个目录项: 成功后返回其目录项地址, 到目录尾时或出错时返回 NULL */
struct dir_entry* readdir(struct dir* dir);
/* 把目录dir的指针dir_pos置0 */
void rewinddir(struct dir* dir);
/* 关闭目录 dir: 成功返回 0, 失败返回-1 */
int32_t closedir(struct dir* dir);
/* 当前工作目录为绝对路径path: 成功则返回0, 失败返回-1 */
int32_t chdir(const char* path);
/* fork子进程：成功后父进程返回子进程 pid，子进程返回0；失败返回 -1 */
int32_t fork();
/* 让进程执行path路径对应的文件，argv为指定的参数 */
int32_t execv(const char* path, const char* argv[]);
/* 进程用来结束调用 */
void exit(int32_t status);
/* 等待子进程调用 exit, 将子进程的退出状态保存到status指向的变量: 成功则返回子进程的pid, 失败则返回 -1 */
int32_t wait(int32_t* status);
/* 显示进程状态 */
void ps();

#endif
