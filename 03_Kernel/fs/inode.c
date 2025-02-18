#include "inode.h"
#include "fs.h"
#include "ide.h"
#include "string.h"
#include "stdint.h"
#include "debug.h"
#include "list.h"
#include "memory.h"
#include "interrupt.h"
#include "thread.h"

extern struct partition* g_curPartion;

/* 用来存储inode位置 */
struct inode_position {
    uint32_t sector_lba;        // inode 所在的扇区号
    uint32_t offset_in_sector;  // inode 在扇区内的字节偏移量
    bool is_spanning_sector;    // inode 是否跨扇区
};

/* 获取inode所在的扇区和扇区内的偏移量 */
static void get_inode_position(struct partition* part, uint32_t inode_no, struct inode_position* inode_pos) {
    /* inode_table在硬盘上是连续的 */
    ASSERT(inode_no < 4096);
    uint32_t inode_table_lba = part->sb->inode_table_lba;

    uint32_t inode_size = sizeof(struct inode);
    uint32_t off_size = inode_no * inode_size;  // 第inode_no号I结点相对于inode_table_lba的字节偏移量
    uint32_t off_sec = off_size / 512;          // 第inode_no号I结点相对于inode_table_lba的扇区偏移量
    uint32_t off_size_in_sec = off_size % 512;  // 待查找的inode所在扇区中的起始地址

    /* 判断此i结点是否跨越2个扇区 */
    uint32_t left_in_sec = 512 - off_size_in_sec;
    if (left_in_sec < inode_size ) {  // 若扇区内剩下的空间不足以容纳一个 inode, 必然是I结点跨越了2个扇区
        inode_pos->is_spanning_sector = true;
    } else {
        inode_pos->is_spanning_sector = false;
    }
    inode_pos->sector_lba = inode_table_lba + off_sec;
    inode_pos->offset_in_sector = off_size_in_sec;
}

/* 将 inode 写入到分区 part */
void write_inode_to_deskpart(struct partition* part, struct inode* inode, void* io_buf) {
    uint8_t inode_no = inode->i_no;
    struct inode_position inode_pos;
    /* 获取 inode 的位置信息 */
    get_inode_position(part, inode_no, &inode_pos);
    ASSERT(inode_pos.sector_lba <= (part->start_lba + part->sec_cnt));
   
    /* 硬盘中的inode中的成员inode_tag和i_open_cnts是不需要的,
    ** 它们只在内存中记录链表位置和被多少进程共享 */
    struct inode pure_inode;
    memcpy(&pure_inode, inode, sizeof(struct inode));

    /* 以下inode的三个成员只存在于内存中, 现在将inode同步到硬盘, 清掉这三项即可 */
    pure_inode.i_open_cnts = 0;
    pure_inode.write_deny = false;
    pure_inode.inode_tag.prev = NULL;
    pure_inode.inode_tag.next = NULL;

    char* inode_buf = (char*)io_buf;
    if (inode_pos.is_spanning_sector) {  // 若是跨了两个扇区, 就要读出两个扇区再写入两个扇区
        /* 读写硬盘是以扇区为单位,若写入的数据小于一扇区,要将原硬盘上的内容先读出来再和新数据拼成一扇区后再写入  */
        ide_read(part->my_disk, inode_pos.sector_lba, inode_buf, 2);  // inode在format中写入硬盘时是连续写入的,所以读入2块扇区
        /* 开始将待写入的inode拼入到这2个扇区中的相应位置 */
        memcpy((inode_buf + inode_pos.offset_in_sector), &pure_inode, sizeof(struct inode));
        /* 将拼接好的数据再写入磁盘 */
        ide_write(part->my_disk, inode_pos.sector_lba, inode_buf, 2);
    } else {			    // 若只是一个扇区
        ide_read(part->my_disk, inode_pos.sector_lba, inode_buf, 1);
        memcpy((inode_buf + inode_pos.offset_in_sector), &pure_inode, sizeof(struct inode));
        ide_write(part->my_disk, inode_pos.sector_lba, inode_buf, 1);
    }
}

/* 将 inode 信息从硬盘分区 part 上删除 */
void delete_inode_from_deskpart(struct partition* part, uint32_t inode_no, void* io_buf) {
    ASSERT(inode_no < 4096);
    struct inode_position inode_pos;
    get_inode_position(part, inode_no, &inode_pos);
    ASSERT(inode_pos.sector_lba <= (part->start_lba + part->sec_cnt));
   
    char* inode_buf = (char*)io_buf;
    if (inode_pos.is_spanning_sector) {  // inode跨扇区
        /* 将原硬盘上的内容先读出来 */
        ide_read(part->my_disk, inode_pos.sector_lba, inode_buf, 2);
        /* 将inode_buf清0 */
        memset((inode_buf + inode_pos.offset_in_sector), 0, sizeof(struct inode));
        /* 用清 0 的内存数据覆盖磁盘 */
        ide_write(part->my_disk, inode_pos.sector_lba, inode_buf, 2);
    } else {
        /* 将原硬盘上的内容先读出来 */
        ide_read(part->my_disk, inode_pos.sector_lba, inode_buf, 1);
        /* 将inode_buf清0 */
        memset((inode_buf + inode_pos.offset_in_sector), 0, sizeof(struct inode));
        /* 用清0的内存数据覆盖磁盘 */
        ide_write(part->my_disk, inode_pos.sector_lba, inode_buf, 1);
    }
}

/* 根据 inode 结点号返回相应的 inode 结点 */
struct inode* inode_open(struct partition* part, uint32_t inode_no) {
    /* 先在已打开inode链表中找inode,此链表是为提速创建的缓冲区 */
    struct list_elem* elem = part->open_inodes.head.next;
    struct inode* inode_found;
    while (elem != &part->open_inodes.tail) {
        inode_found = GET_ENTRYPTR_FROM_LISTTAG(struct inode, inode_tag, elem);
        if (inode_found->i_no == inode_no) {
            inode_found->i_open_cnts++;
            return inode_found;
        }
        elem = elem->next;
    }

    /* open_inodes 链表中找不到, 从硬盘上读入此 inode 并加入到此链表 */
    struct inode_position inode_pos;

    /* inode位置信息会存入inode_pos, 包括inode所在扇区地址和扇区内的字节偏移量 */
    get_inode_position(part, inode_no, &inode_pos);

    /* 为使通过sys_malloc创建的新inode被所有任务共享,
    ** 需要将inode置于内核空间, 故需要临时
    ** 将cur_pbc->pgdir置为NULL */
    struct PCB_INFO* cur = get_curthread_pcb();
    uint32_t* cur_pagedir_bak = cur->pgdir;
    cur->pgdir = NULL;
    inode_found = (struct inode*)sys_malloc(sizeof(struct inode));
    /* 恢复pgdir */
    cur->pgdir = cur_pagedir_bak;

    char* inode_buf;
    if (inode_pos.is_spanning_sector) {	// 跨扇区的情况
        inode_buf = (char*)sys_malloc(1024);
        ide_read(part->my_disk, inode_pos.sector_lba, inode_buf, 2);
    } else {
        inode_buf = (char*)sys_malloc(512);
        ide_read(part->my_disk, inode_pos.sector_lba, inode_buf, 1);
    }
    memcpy(inode_found, inode_buf + inode_pos.offset_in_sector, sizeof(struct inode));

    /* 将 inode 插入队列 */
    list_push(&part->open_inodes, &inode_found->inode_tag);
    inode_found->i_open_cnts = 1;
    sys_free(inode_buf);
    return inode_found;
}

/* 关闭 inode 或减少 inode 的打开数 */
void inode_close(struct inode* inode) {
    /* 若没有进程再打开此文件,将此inode去掉并释放空间 */
    enum interrupt_status old_status =  get_interrupt_status();
    interrupt_disable();
    if (--inode->i_open_cnts == 0) {
        list_remove(&inode->inode_tag);
        /* inode_open时为实现inode被所有进程共享,
        ** 已经在sys_malloc为inode分配了内核空间,
        ** 释放inode时也要确保释放的是内核内存池 */
        struct PCB_INFO* cur = get_curthread_pcb();
        uint32_t* cur_pagedir_bak = cur->pgdir;
        cur->pgdir = NULL;
        sys_free(inode);
        cur->pgdir = cur_pagedir_bak;
    }
    set_interrupt_status(old_status);
}

/* 回收 inode 的数据块和 inode 本身 */
void inode_release(struct partition* part, uint32_t inode_no) {
    struct inode* inode_to_del = inode_open(part, inode_no);
    ASSERT(inode_to_del->i_no == inode_no);

    /* 回收 inode 占用的所有块 */
    uint8_t block_idx = 0;
    uint8_t block_cnt = 12;
    uint32_t block_bitmap_idx;
    uint32_t all_blocks[140] = {0};  // 12 个直接块 + 128 个间接块
    /* 先将前12个直接块存入 all_blocks */
    while (block_idx < block_cnt) {
        all_blocks[block_idx] = inode_to_del->i_sectors[block_idx];
        block_idx++;
    }
    /* 如果一级间接块表存在, 将其128个间接块读到 all_blocks[12~], 并释放一级间接块表所占的扇区 */
    if (inode_to_del->i_sectors[12] != 0) {
        ide_read(part->my_disk, inode_to_del->i_sectors[12], all_blocks + 12, 1);
        block_cnt = 140;
        /* 回收一级间接块表占用的扇区 */
        block_bitmap_idx = inode_to_del->i_sectors[12] - part->sb->data_start_lba;
        ASSERT(block_bitmap_idx > 0);
        bitmap_set(&part->block_bitmap, block_bitmap_idx, 0);
        fs_bitmap_sync(g_curPartion, block_bitmap_idx, PART_BLOCK_BITMAP);
    }

    /* 根据 all_blocks 中的 inode 编号逐个回收 */
    block_idx = 0;
    while (block_idx < block_cnt) {
        if (all_blocks[block_idx] != 0) {
            block_bitmap_idx = all_blocks[block_idx] - part->sb->data_start_lba;
            ASSERT(block_bitmap_idx > 0);
            bitmap_set(&part->block_bitmap, block_bitmap_idx, 0);
            fs_bitmap_sync(g_curPartion, block_bitmap_idx, PART_BLOCK_BITMAP);
        }
        block_idx++; 
    }

    /* 回收该 inode 所占用的 inode */
    bitmap_set(&part->inode_bitmap, inode_no, 0);  
    fs_bitmap_sync(g_curPartion, inode_no, PART_INODE_BITMAP);
}

/* 初始化 inode */
void inode_init(uint32_t inode_no, struct inode* new_inode) {
    new_inode->i_no = inode_no;
    new_inode->i_size = 0;
    new_inode->i_open_cnts = 0;
    new_inode->write_deny = false;

    /* 初始化块索引数组 i_sector */
    uint8_t sec_idx = 0;
    while (sec_idx < 13) {
        new_inode->i_sectors[sec_idx] = 0;
        sec_idx++;
    }
}