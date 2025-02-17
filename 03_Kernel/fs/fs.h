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

/* 文件类型 */
enum file_type {
    FT_UNKNOWN,     // 不支持的文件类型
    FT_REGULAR,     // 普通文件
    FT_DIRECTORY,   // 目录
    FT_TYPE_MAX
};

void file_system_init(void);

#endif
