#ifndef _DEVICE_IDE_H_
#define _DEVICE_IDE_H_

#include "stdint.h"
#include "semaphore.h"
#include "bitmap.h"
#include "list.h"
#include "lock.h"

/* 分区结构 */
struct partition {
    uint32_t start_lba;          // 起始扇区
    uint32_t sec_cnt;            // 扇区数
    struct disk* my_disk;        // 分区所属的硬盘
    struct list_elem part_tag;   // 用于队列中的标记
    char name[8];                // 分区名称
    struct super_block* sb;      // 本分区的超级块
    struct bitmap block_bitmap;  // 块位图
    struct bitmap inode_bitmap;  // i结点位图
    struct list open_inodes;     // 本分区打开的i结点队列
};

/* 硬盘结构 */
struct disk {
    char name[8];                       // 本硬盘的名称，如sda等
    struct ide_channel* my_channel;     // 此块硬盘归属于哪个ide通道
    uint8_t dev_no;                     // 本硬盘是主0还是从1
    struct partition prim_parts[4];     // 主分区顶多是4个
    struct partition logic_parts[8];    // 逻辑分区数量上限, 本操作系统设置为8个
};

/* ata通道结构 */
struct ide_channel {
    char name[8];                // 本ata通道名称, 如ata0,也被叫做ide0
    uint16_t port_base;          // 本通道的起始端口号
    uint8_t irq_no;              // 本通道所用的中断号
    struct lock lock;
    bool expecting_intr;         // 向硬盘发完命令后等待来自硬盘的中断
    struct semaphore disk_done;  // 硬盘处理完成.线程用这个信号量来阻塞自己，由硬盘完成后产生的中断将线程唤醒
    struct disk devices[2];      // 一个通道上连接两个硬盘，一主一从
};

void ide_init();

#endif
