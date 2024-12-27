#include "stdint.h"
#include "memory.h"
#include "bitmap.h"
#include "print.h"

struct memory_poll kernel_memory_pool;
struct memory_poll user_memory_pool;
struct memory_poll kernel_virtual_addr;  // 内核虚拟地址分配管理


void memory_pool_init(uint32_t all_memory_bytes)
{
    put_str("\nmemory pool init start!\n");
    uint32_t used_memory_bytes = 0x100000 + PAGE_SIZE * 256;            // 已用内存：低端1MB+页表占用部分
    uint32_t free_memory_bytes = all_memory_bytes - used_memory_bytes;  // 空闲的内存
    uint32_t free_pages = free_memory_bytes / PAGE_SIZE;                // 空闲内存可分页数
    uint32_t kernel_pages = free_pages / 2;                             // 内核内存池页数
    uint32_t user_pages = free_pages - kernel_pages;                    // 用户内存池页数

    /* 内核内存池 */
    kernel_memory_pool.addr_start = 0x100000 + PAGE_SIZE * 256;
    kernel_memory_pool.size = kernel_pages * PAGE_SIZE;
    kernel_memory_pool.pool_bitmap.bits = MEMORY_BITMAP_ADDR;
    kernel_memory_pool.pool_bitmap.bytes_num = kernel_pages / 8;
    bitmap_init(&(kernel_memory_pool.pool_bitmap));
    put_str("\nkernel pool init: ");
    put_str("\nphy addr: ");
    put_int(kernel_memory_pool.addr_start);
    put_str("\nsize: ");
    put_int(kernel_memory_pool.size);
    put_str("\npool bitmap bits addr: ");
    put_int(kernel_memory_pool.pool_bitmap.bits);
    put_str("\npool bitmap size(byte): ");
    put_int(kernel_memory_pool.pool_bitmap.bytes_num);

    /* 内核内存池 */
    user_memory_pool.addr_start = kernel_memory_pool.addr_start + kernel_memory_pool.size;
    user_memory_pool.size = user_pages * PAGE_SIZE;
    user_memory_pool.pool_bitmap.bits = kernel_memory_pool.pool_bitmap.bits + kernel_memory_pool.pool_bitmap.bytes_num;
    user_memory_pool.pool_bitmap.bytes_num = user_pages / 8;
    bitmap_init(&(user_memory_pool.pool_bitmap));
    put_str("\nuser pool init: ");
    put_str("\nphy addr: ");
    put_int(user_memory_pool.addr_start);
    put_str("\nsize: ");
    put_int(user_memory_pool.size);
    put_str("\npool bitmap bits addr: ");
    put_int(user_memory_pool.pool_bitmap.bits);
    put_str("\npool bitmap size(byte): ");
    put_int(user_memory_pool.pool_bitmap.bytes_num);

    /* 内核虚拟地址位图初始化 */
    kernel_virtual_addr.addr_start = KERNEL_HEAP_START;
    kernel_virtual_addr.pool_bitmap.bits = user_memory_pool.pool_bitmap.bits + user_memory_pool.pool_bitmap.bytes_num;
    kernel_virtual_addr.pool_bitmap.bytes_num = kernel_memory_pool.pool_bitmap.bytes_num;
    bitmap_init(&(kernel_virtual_addr.pool_bitmap));
    put_str("\nkernel virtual addr init: ");
    put_str("\naddr start: ");
    put_int(kernel_virtual_addr.addr_start);
    put_str("\npool bitmap bits addr: ");
    put_int(kernel_virtual_addr.pool_bitmap.bits);
    put_str("\npool bitmap size(byte): ");
    put_int(kernel_virtual_addr.pool_bitmap.bytes_num);

    put_str("\nmemory pool init done!\n");
}

void memory_init()
{
    uint32_t memory_bytes = 32 * 1024 * 1024;
    memory_pool_init(memory_bytes);
}