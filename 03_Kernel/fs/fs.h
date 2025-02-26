#ifndef __FS_FS_H__
#define __FS_FS_H__

#include "stdint.h"
#include "ide.h"

/* 每个分区所支持最大创建的文件数 */
#define MAX_FILES_PER_PART (4096)
/* 每扇区的位数 */
#define BITS_PER_SECTOR (4096)
/* 扇区字节大小 */
#define SECTOR_SIZE (512)
/* 块字节大小 */
#define BLOCK_SIZE (SECTOR_SIZE)
/* 路径名称最大长度 */
#define MAX_PATH_LEN (512)

/* 文件类型 */
enum file_type {
    FT_UNKNOWN,     // 不支持的文件类型
    FT_REGULAR,     // 普通文件
    FT_DIRECTORY,   // 目录
    FT_TYPE_MAX
};

/* 文件操作权限 */
enum oflag {
    O_RDONLY,    // 只读
    O_WRONLY,    // 只写
    O_RDWR,      // 读写
    O_CREAT = 4  // 创建
};

/* 用来记录查找文件过程中已找到的上级路径 */
struct path_search_record {
    char searched_path[MAX_PATH_LEN];  // 查找过程中的父路径
    struct dir* parent_dir;            // 文件或目录所在的直接父目录
    enum file_type filetype;           // 找到的是普通文件还是目录,找不到将为未知类型(FT_UNKNOWN)
};

/* 文件属性结构体 */
struct stat_info {
    uint32_t st_ino;              // inode 编号
    uint32_t st_size;             // 尺寸
    enum file_type st_filetype;   // 文件类型
};

/* 分区位图类型枚举 */
enum partition_bitmap_type {
   PART_INODE_BITMAP,  // inode位图
   PART_BLOCK_BITMAP   // 块位图
};

void file_system_init(void);
/* 将内存中 bitmap 第 bit_idx 位所在的 512 字节同步到硬盘 */
void fs_bitmap_sync(struct partition* part, uint32_t bit_idx, enum partition_bitmap_type btmp_type);
/* 分配一个i结点, 返回i结点号 */
int32_t fs_inode_bitmap_alloc(struct partition* part);
/* 分配一个扇区, 返回其扇区地址 */
int32_t fs_block_bitmap_alloc(struct partition* part);
/* 将最上层路径名称解析出来 */
char* path_parse(char* pathname, char* dirname);
/* 打开或创建文件成功后, 返回文件描述符, 否则返回-1 */
int32_t sys_open(const char* pathname, uint8_t flags);
/* 关闭文件描述符fd指向的文件, 成功返回0, 否则返回-1 */
int32_t sys_close(int32_t fd);
/* 将buf中连续 count 个字节写入文件描述符fd, 成功则返回写入的字节数, 失败返回-1 */
int32_t sys_write(int32_t fd, const void* buf, uint32_t count);
/* 从文件描述符fd指向的文件中读取count个字节到buf, 若成功则返回读出的字节数, 到文件尾则返回-1 */
int32_t sys_read(int32_t fd, void* buf, uint32_t count);
/* 向屏幕输出一个字符 */
void sys_putchar(char c);
/* 获取当前工作目录，成功返回 路径buf，失败返回 NULL，返回的 buf 由用户释放 */
char* sys_getcwd();
/* 根据绝对路径 pathname 创建目录：成功返回 0；失败返回 -1 */
int32_t sys_mkdir(const char* pathname);
/* 在buf中填充文件结构相关信息: 成功时返回0, 失败返回-1 */
int32_t sys_stat(const char* path, struct stat_info* buf);
/* 打开目录：成功后返回目录指针, 失败返回 NULL */
struct dir* sys_opendir(const char* name);
/* 读取目录 dir 的1个目录项: 成功后返回其目录项地址, 到目录尾时或出错时返回 NULL */
struct dir_entry* sys_readdir(struct dir* dir);
/* 当前工作目录为绝对路径path: 成功则返回0, 失败返回-1 */
int32_t sys_chdir(const char* path);
/* 把目录dir的指针dir_pos置0 */
void sys_rewinddir(struct dir* dir);
/* 关闭目录 dir: 成功返回 0, 失败返回-1 */
int32_t sys_closedir(struct dir* dir);

#endif
