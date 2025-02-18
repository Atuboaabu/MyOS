#ifndef __FS_FILE_H__
#define __FS_FILE_H__

#include "stdint.h"
#include "inode.h"

/* 文件结构 */
struct file {
    uint32_t fd_pos;  // 记录当前文件操作的偏移地址
    uint32_t fd_flag;
    struct inode* fd_inode;
};

/* 标准输入输出描述符 */
#define stdin (0)   // 标准输入
#define stdout (1)  // 标准输出
#define stderr (2)  // 标准错误

/* 最大可打开文件数 */
#define MAX_FILES (32)

#endif
