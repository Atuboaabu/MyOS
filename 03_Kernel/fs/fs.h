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

#endif
