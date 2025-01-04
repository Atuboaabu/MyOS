#include "stdint.h"
#include "memory.h"
#include "bitmap.h"
#include "print.h"
#include "string.h"
#include "debug.h"

struct memory_poll kernel_memory_pool;
struct memory_poll user_memory_pool;
struct virtual_addr_pool kernel_virtual_addr;  // 内核虚拟地址分配管理

/* 在虚拟内存地址池中申请地址：成功则返回虚拟页的起始地址, 失败则返回NULL */
static void* vaddr_get(enum pool_flag pf, uint32_t cnt) {
    uint32_t vaddr_start = NULL;
    uint32_t bit_start = -1;
    uint32_t bitcnt = 0;
    if (pf == POOL_FLAG_KERNEL) {
        /* 1、bitmap申请，查看是否有有连续cnt个空闲的虚拟地址 */
        bit_start  = bitmap_apply(&kernel_virtual_addr.pool_bitmap, cnt);
        if (bit_start == -1) {
            return NULL;
        }
        /* 2、置位bitmap */
        while(bitcnt < cnt) {
            bitmap_set(&kernel_virtual_addr.pool_bitmap, bit_start + bitcnt, 1);
            bitcnt++;
        }
        /* 3、计算申请到的虚拟地址用于返回 */
        vaddr_start = kernel_virtual_addr.addr_start + bit_start * PAGE_SIZE;
    } else {	
        // 用户内存池,将来实现用户进程再补充
    }
    return (void*)vaddr_start;
}

/* 在 mem_pool 指向的物理内存池中分配1个物理页：成功则返回页框的物理地址；失败则返回 NULL */
static void* palloc(struct memory_poll* mem_pool) {
    /* 判断是否有满足空闲的物理页 */
    int bit_idx = bitmap_apply(&(mem_pool->pool_bitmap), 1);    // 找一个物理页面
    if (bit_idx == -1 ) {
        return NULL;
    }
    /* 置位bitmap并返回申请到的物理地址 */
    bitmap_set(&(mem_pool->pool_bitmap), bit_idx, 1);	// 将此位bit_idx置1
    uint32_t phyaddr = ((bit_idx * PAGE_SIZE) + mem_pool->addr_start);
    return (void*)phyaddr;
}

/* 获取虚拟地址 vaddr 对应的 pte 指针 */
uint32_t* get_pte_ptr(uint32_t vaddr) {
    /* 1、虚拟地址的高10位为 PDE 的索引，通过宏 PDE_INDEX 转换
    *  2、0xFFC00000 可以获取到页表目录第1024项，该项指向的是 页目录表起始地址
    *  3、vaddr & 0xFFC00000 获取虚拟地址对应的页表目录索引，用此索引作 页表索引 找到对应的页表地址
    *  4、PTE_INDEX(vaddr) * 4 即虚拟地址对应的页表项地址便宜
    */
    uint32_t* pte = (uint32_t*)(0xFFC00000 + ((vaddr & 0xFFc00000) >> 10) + PTE_INDEX(vaddr) * 4);
    return pte;
}

/* 获取虚拟地址 vaddr 对应的 pde 的指针 */
uint32_t* get_pde_ptr(uint32_t vaddr) {
    /* 1、虚拟地址的高10位为 PDE 的索引，通过宏 PDE_INDEX 转换
    *  2、0xFFC00000 可以获取到页表目录第1024项，该项指向的是 页目录表起始地址
    *  3、0xFFFFF000 页表目录第1024个页表项指向的地址：即页表目录起始地址
    *  4、(0xFFFFF000) + PDE_INDEX(vaddr) * 4 即虚拟地址对应的页表目录项地址
    */
    uint32_t* pde = (uint32_t*)((0xFFFFF000) + PDE_INDEX(vaddr) * 4);
    return pde;
}

/* 页表中添加虚拟地址 _vaddr 与物理地址 _phyaddr 的映射 */
static void page_table_add(void* _vaddr, void* _phyaddr) {
    uint32_t vaddr = (uint32_t)_vaddr;
    uint32_t phyaddr = (uint32_t)_phyaddr;
    uint32_t* pde = get_pde_ptr(vaddr);
    uint32_t* pte = get_pte_ptr(vaddr);

    /* 先在页目录内判断目录项的P位，若为1,则表示该表已存在 */
    if (*pde & 0x00000001) {  // 判断页目录表是否存在
        ASSERT(!(*pte & 0x00000001));
        *pte = (phyaddr | PG_US_U | PG_RW_W | PG_P_1);    // US=1,RW=1,P=1
    } else {  // 页目录表不存在
        /* 页表中用到的页框一律从内核空间分配 */
        uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_memory_pool);
        *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);

        /* 分配到的物理页地址pde_phyaddr对应的物理内存清0,
        * 避免里面的陈旧数据变成了页表项,从而让页表混乱.
        * 访问到pde对应的物理地址,用pte取高20位便可.
        * 因为pte是基于该pde对应的物理地址内再寻址,
        * 把低12位置0便是该pde对应的物理页的起始*/
        memset((void*)((int)pte & 0xfffff000), 0, PAGE_SIZE);

        ASSERT(!(*pte & 0x00000001));
        *pte = (phyaddr | PG_US_U | PG_RW_W | PG_P_1);      // US=1,RW=1,P=1
    }
}

/* 分配 cnt 个页：成功则返回起始虚拟地址, 失败时返回NULL */
void* malloc_page(enum pool_flag pf, uint32_t cnt) {
    ASSERT(cnt > 0 && cnt < 3840);  // 内核只有15MB内存，最多有 15MB / 4KB = 3840个页
    /* 1、先申请虚拟地址 */
    void* vaddr_start = vaddr_get(pf, cnt);
    if (vaddr_start == NULL) {
        return NULL;
    }

   uint32_t virtual_addr = (uint32_t)vaddr_start;
   uint32_t page_cnt = cnt;
   struct pool* mem_pool = (pf == POOL_FLAG_KERNEL) ? &kernel_memory_pool : &user_memory_pool;

    /* 2、申请物理地址并完成虚拟地址和物理地址在页表中的映射 */
    while (page_cnt-- > 0) {
        void* phyaddr = palloc(mem_pool);
        if (phyaddr == NULL) {  // 失败时要将曾经已申请的虚拟地址和物理页全部回滚，在将来完成内存回收时再补充
            return NULL;
        }
        page_table_add((void*)virtual_addr, phyaddr); // 在页表中做映射 
        virtual_addr += PAGE_SIZE;		 // 下一个虚拟页
    }
    return vaddr_start;
}

/* 从内核申请 cnt 个内存页：成功则返回其虚拟地址，失败返回NULL */
void* get_kernel_pages(uint32_t cnt) {
   void* vaddr =  malloc_page(POOL_FLAG_KERNEL, cnt);
   if (vaddr != NULL) {
      memset(vaddr, 0, cnt * PAGE_SIZE);
   }
   return vaddr;
}

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
    uint32_t memory_bytes = 32 * 1024 * 1024;  // 总共有32MB内存
    memory_pool_init(memory_bytes);
}