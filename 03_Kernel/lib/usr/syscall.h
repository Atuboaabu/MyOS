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
    SYS_STAT,
    SYS_OPENDIR,
    SYS_READDIR,
    SYS_REWINDDIR,
    SYS_CLOSEDIR,
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
/* 打开目录：成功后返回目录指针, 失败返回 NULL */
struct dir* opendir(const char* name);
/* 读取目录 dir 的1个目录项: 成功后返回其目录项地址, 到目录尾时或出错时返回 NULL */
struct dir_entry* readdir(struct dir* dir);
/* 把目录dir的指针dir_pos置0 */
void rewinddir(struct dir* dir);
/* 关闭目录 dir: 成功返回 0, 失败返回-1 */
int32_t closedir(struct dir* dir);

#endif
