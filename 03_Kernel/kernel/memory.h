#ifndef _KERNEL_MEMORY_H_
#define _KERNEL_MEMORY_H_

#include "stdint.h"
#include "bitmap.h"

#define PAGE_SIZE           (4096)
#define MEMORY_BITMAP_ADDR  (0xC009A000)
#define KERNEL_HEAP_START   (0xC0200000)

/* 页表属性定义 */
#define	 PG_P_1	  (1)  // 页表项或页目录项存在属性位
#define	 PG_P_0	  (0)  // 页表项或页目录项存在属性位
#define	 PG_RW_R  (0)  // R/W 属性位值, 读/执行
#define	 PG_RW_W  (2)  // R/W 属性位值, 读/写/执行
#define	 PG_US_S  (0)  // U/S 属性位值, 系统级
#define	 PG_US_U  (4)  // U/S 属性位值, 用户级

#define PDE_INDEX(vaddr) ((vaddr & 0xFFC00000) >> 22)
#define PTE_INDEX(vaddr) ((vaddr & 0x003FF000) >> 12)

/* 内存池标记,用于判断用哪个内存池 */
enum pool_flag {
   POOL_FLAG_KERNEL = 1,    // 内核内存池
   POOL_FLAG_USER   = 2     // 用户内存池
};

struct memory_poll {
    uint32_t addr_start;        // 物理地址起始位置
    uint32_t size;              // 内存池大小（byte)
    struct bitmap pool_bitmap;  // 内存池对应的位图，用于管理物理内存
};

struct virtual_addr_pool {
    uint32_t addr_start;       // 虚拟地址起始位置
    struct bitmap pool_bitmap; // 虚拟地址对应的位图，用于管理虚拟地址
};

void memory_init();
void* get_kernel_pages(uint32_t cnt);

#endif
