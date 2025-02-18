#ifndef __FS_INODE_H__
#define __FS_INODE_H__
#include "stdint.h"
#include "list.h"
#include "ide.h"

/* inode结构 */
struct inode {
    uint32_t i_no;           // inode编号
    uint32_t i_size;         // inode是文件时, i_size是指文件大小; inode是目录时, i_size是指该目录下所有目录项大小之和
    uint32_t i_open_cnts;    // 记录此文件被打开的次数
    bool write_deny;         // 写文件不能并行,进程写文件前检查此标识
    uint32_t i_sectors[13];  // i_sectors[0-11]是直接块, i_sectors[12]用来存储一级间接块指针
    struct list_elem inode_tag;
};

/* 初始化 inode */
void inode_init(uint32_t inode_no, struct inode* new_inode);
/* 释放 inode 的数据块和 inode 本身 */
void inode_release(struct partition* part, uint32_t inode_no);
/* 根据 inode 结点号返回相应的 inode 结点 */
struct inode* inode_open(struct partition* part, uint32_t inode_no);
/* 关闭 inode 或减少 inode 的打开数 */
void inode_close(struct inode* inode);
/* 将 inode 写入到分区 part */
void write_inode_to_deskpart(struct partition* part, struct inode* inode, void* io_buf);
/* 将 inode 信息从硬盘分区 part 上删除 */
void delete_inode_from_deskpart(struct partition* part, uint32_t inode_no, void* io_buf);

#endif
