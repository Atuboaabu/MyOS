#ifndef _KERNEL_MEMORY_H_
#define _KERNEL_MEMORY_H_

#include "stdint.h"
#include "bitmap.h"

#define PAGE_SIZE           (4096)
#define MEMORY_BITMAP_ADDR  (0xC009A000)
#define KERNEL_HEAP_START   (0xC0100000)


struct memory_poll {
    uint32_t addr_start;     // 物理地址起始位置
    uint32_t size;              // 内存池大小（byte)
    struct bitmap pool_bitmap;  // 内存池对应的位图，用于管理物理内存
};

void memory_init();

#endif
