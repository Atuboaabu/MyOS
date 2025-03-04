#include "stdint.h"
#include "memory.h"
#include "bitmap.h"
#include "print.h"
#include "string.h"
#include "debug.h"
#include "thread.h"

/* 内存仓库 mem_arena 元信息 */
struct mem_arena {
    uint32_t cnt;                               // 此 mem_arena 空闲的 mem_block 数量，isPage 为 true 表示分配的页数
    bool isPage;                                // true 表示是页分配，false 表示是block分配
    struct mem_block_pool* mem_block_pool_ptr;  // 此 mem_arena 关联的 mem_block_pool
};

struct memory_poll kernel_memory_pool;
struct memory_poll user_memory_pool;
struct virtual_addr_pool kernel_virtual_addr;  // 内核虚拟地址分配管理
/* 内核内存池数组 */
static struct mem_block_pool kernel_memblock_pools[MEMORY_POOL_COUNT];

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
        struct PCB_INFO* cur_thread_pcb = get_curthread_pcb();
        /* 1、bitmap申请，查看是否有有连续cnt个空闲的虚拟地址 */
        bit_start = bitmap_apply(&(cur_thread_pcb->user_virtual_addr.pool_bitmap), cnt);
        if (bit_start == -1) {
            return NULL;
        }
        /* 2、置位bitmap */
        while(bitcnt < cnt) {
            bitmap_set(&(cur_thread_pcb->user_virtual_addr.pool_bitmap), bit_start + bitcnt, 1);
            bitcnt++;
        }
        /* 3、计算申请到的虚拟地址用于返回 */
        vaddr_start = cur_thread_pcb->user_virtual_addr.addr_start + bit_start * PAGE_SIZE;
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
    uint32_t* pte = (uint32_t*)(0xFFC00000 + ((vaddr & 0xFFC00000) >> 10) + PTE_INDEX(vaddr) * 4);
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

/* 页表中添加虚拟地址 virtual_addr 与物理地址 _phyaddr 的映射 */
static void page_table_add(void* virtual_addr, void* _phyaddr) {
    uint32_t vaddr = (uint32_t)virtual_addr;
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
   struct memory_poll* mem_pool = (pf == POOL_FLAG_KERNEL) ? &kernel_memory_pool : &user_memory_pool;

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
    lock_acquire(&(kernel_memory_pool.lock));
    void* vaddr =  malloc_page(POOL_FLAG_KERNEL, cnt);
    if (vaddr != NULL) {
        memset(vaddr, 0, cnt * PAGE_SIZE);
    }
    lock_release(&(kernel_memory_pool.lock));
    return vaddr;
}

/* 从用户内存申请 cnt 个内存页：成功则返回其虚拟地址，失败返回NULL */
void* get_user_pages(uint32_t cnt) {
    lock_acquire(&(user_memory_pool.lock));
    void* vaddr =  malloc_page(POOL_FLAG_USER, cnt);
    if (vaddr != NULL) {
        memset(vaddr, 0, cnt * PAGE_SIZE);
    }
    lock_release(&(user_memory_pool.lock));
    return vaddr;
}

/* 将地址 vaddr 与 pf 池中的物理地址关联, 仅支持一页空间分配 */
void* bind_vaddr_with_mempool(enum pool_flag pf, uint32_t vaddr) {
    struct memory_poll* mem_pool = ((pf == POOL_FLAG_KERNEL) ? &kernel_memory_pool : &user_memory_pool);
    lock_acquire(&(mem_pool->lock));

    /* 1、将虚拟地址对应的位图置1 */
    struct PCB_INFO* cur_thread_pcb = get_curthread_pcb();
    uint32_t bit_idx = -1;

    /* 若当前是用户进程申请用户内存,就修改用户进程自己的虚拟地址位图 */
    if (cur_thread_pcb->pgdir != NULL && pf == POOL_FLAG_USER) {
        bit_idx = (vaddr - cur_thread_pcb->user_virtual_addr.addr_start) / PAGE_SIZE;
        ASSERT(bit_idx > 0);
        bitmap_set(&(cur_thread_pcb->user_virtual_addr.pool_bitmap), bit_idx, 1);
    } else if (cur_thread_pcb->pgdir == NULL && pf == POOL_FLAG_KERNEL){
        /* 如果是内核线程申请内核内存,就修改kernel_vaddr. */
        bit_idx = (vaddr - kernel_virtual_addr.addr_start) / PAGE_SIZE;
        ASSERT(bit_idx > 0);
        bitmap_set(&(kernel_virtual_addr.pool_bitmap), bit_idx, 1);
    } else {
        put_str("get_a_page: Wrong operation!\n");
    }

    void* page_phyaddr = palloc(mem_pool);
    if (page_phyaddr == NULL) {
        return NULL;
    }
    page_table_add((void*)vaddr, page_phyaddr);
    lock_release(&(mem_pool->lock));
    return (void*)vaddr;
}

/* 申请一页内存与vaddr绑定，但不设置vaddr的bitmap */
void* get_a_page_without_set_vaddrbmp(enum pool_flag pf, uint32_t vaddr) {
    struct memory_poll* mem_pool = ((pf == POOL_FLAG_KERNEL) ? &kernel_memory_pool : &user_memory_pool);
    lock_acquire(&(mem_pool->lock));

    void* page_phyaddr = palloc(mem_pool);
    if (page_phyaddr == NULL) {
        return NULL;
    }
    page_table_add((void*)vaddr, page_phyaddr);
    lock_release(&(mem_pool->lock));
    return (void*)vaddr;
}

/* 获取虚拟地址映射到的物理地址 */
uint32_t vaddr_to_phyaddr(uint32_t vaddr) {
    uint32_t* pte = get_pte_ptr(vaddr);
    return ((*pte & 0xFFFFF000) + (vaddr & 0x00000FFF));
}

/* 获取 mem_arena 中的第 index 个 mem_block */
struct mem_block* get_arena_memblock(struct mem_arena* mem_arena, uint32_t index) {
    return (struct mem_block*)((uint32_t)mem_arena + sizeof(struct mem_arena)
            + index * mem_arena->mem_block_pool_ptr->mem_block_size);
}

/* 根据 memblock 获取其对应的 mem_arena 地址 */
struct mem_arena* get_arena_by_memblock(struct mem_block* block) {
    return (struct mem_arena*)((uint32_t)block & 0xFFFFF000);
}

/* 在堆中申请内存 */
void* sys_malloc(uint32_t size) {
    enum pool_flag pf;
    struct memory_poll* mem_pool;
    uint32_t pool_size;
    struct mem_block_pool* block_pool;
    struct PCB_INFO* cur_thread_pcb = get_curthread_pcb();

    if (cur_thread_pcb->pgdir == NULL) {  // 内核线程
        pf = POOL_FLAG_KERNEL; 
        pool_size = kernel_memory_pool.size;
        mem_pool = &kernel_memory_pool;
        block_pool = kernel_memblock_pools;
    } else {  // 用户进程
        pf = POOL_FLAG_USER;
        pool_size = user_memory_pool.size;
        mem_pool = &user_memory_pool;
        block_pool = cur_thread_pcb->user_memblock_pools;
    }

    /* 若申请的内存不在内存池容量范围内则直接返回NULL */
    if (size < 0 || size > pool_size) {
        return NULL;
    }

    struct mem_arena* mem_arena;
    struct mem_block* block;
    lock_acquire(&mem_pool->lock);

    /* 超过最大内存块1024B, 分配页框 */
    if (size > 1024) {
        // 需要分配的页数(向上取整)
        uint32_t page_cnt = (size + sizeof(struct mem_arena) + PAGE_SIZE - 1) / PAGE_SIZE;
        mem_arena = malloc_page(pf, page_cnt);
        if (mem_arena != NULL) {
            memset(mem_arena, 0, page_cnt * PAGE_SIZE); 
            /* 页分配：mem_block_pool_ptr 置为 NULL, cnt 置为页框数, isPage 置为 true */
            mem_arena->mem_block_pool_ptr = NULL;
            mem_arena->cnt = page_cnt;
            mem_arena->isPage = true;
            lock_release(&mem_pool->lock);
            return (void*)(mem_arena + 1);  // 跨过arena大小，把剩下的内存返回
        } else { 
            lock_release(&mem_pool->lock);
            return NULL; 
        }
    } else {  // 申请内存小于等于 1024B，按 block 分配
        uint8_t pool_idx;
        /* 从内存块描述符中匹配合适的内存块规格 */
        for (pool_idx = 0; pool_idx < MEMORY_POOL_COUNT; pool_idx++) {
            if (size <= block_pool[pool_idx].mem_block_size) {  // 从小往大进行匹配，找到后退出
                break;
            }
        }

        /* 若对应大小内存池无空闲内存块，则申请一个页框 */
        if (list_empty(&(block_pool[pool_idx].mem_block_list))) {
            mem_arena = malloc_page(pf, 1);       // 分配1页框做为arena
            if (mem_arena == NULL) {
                lock_release(&mem_pool->lock);
                return NULL;
            }
            memset(mem_arena, 0, PAGE_SIZE);

            
            mem_arena->mem_block_pool_ptr = &block_pool[pool_idx];
            mem_arena->isPage = false;
            mem_arena->cnt = block_pool[pool_idx].mem_block_cnt;

            uint32_t block_idx;
            /* 将arena拆分成内存块, 并添加到内存块 mem_block_list 中 */
            for (block_idx = 0; block_idx < block_pool[pool_idx].mem_block_cnt; block_idx++) {
                block = get_arena_memblock(mem_arena, block_idx);
                ASSERT(!elem_find(&(mem_arena->mem_block_pool_ptr->mem_block_list), &(block->mem_elem)));
                list_append(&(mem_arena->mem_block_pool_ptr->mem_block_list), &(block->mem_elem));	
            }
        }

        /* 开始分配内存块 */
        block = GET_ENTRYPTR_FROM_LISTTAG(struct mem_block, mem_elem, list_pop(&(block_pool[pool_idx].mem_block_list)));
        memset(block, 0, block_pool[pool_idx].mem_block_size);

        mem_arena = get_arena_by_memblock(block);  // 获取内存块 block 所在的arena
        mem_arena->cnt--;                          // 将此 mem_arena 中的空闲内存块数减 1
        lock_release(&mem_pool->lock);
        return (void*)block;
   }
}

/* 将物理地址 phy_addr 对应的页回收到物理内存池 */
void phyaddr_free(uint32_t phy_addr) {
    struct memory_poll* mem_pool;
    uint32_t bit_idx = 0;
    if (phy_addr >= user_memory_pool.addr_start) {     // 用户物理内存池
        mem_pool = &user_memory_pool;
        bit_idx = (phy_addr - user_memory_pool.addr_start) / PAGE_SIZE;
    } else {	  // 内核物理内存池
        mem_pool = &kernel_memory_pool;
        bit_idx = (phy_addr - kernel_memory_pool.addr_start) / PAGE_SIZE;
    }
    bitmap_set(&mem_pool->pool_bitmap, bit_idx, 0);	 // 将位图中该位清0
}

/* 去掉页表中虚拟地址 vaddr 的映射,只去掉 vaddr 对应的pte */
static void remove_pte_from_pagetable(uint32_t vaddr) {
    uint32_t* pte = get_pte_ptr(vaddr);
    *pte &= ~PG_P_1;  // 将页表项pte的P位置0
    asm volatile ("invlpg %0"::"m" (vaddr):"memory");  //更新tlb
}

/* 在虚拟地址池中释放以_vaddr起始的连续pg_cnt个虚拟页地址 */
static void remove_virtual_addr(enum pool_flag pf, void* virtual_addr, uint32_t pg_cnt) {
    uint32_t bit_idx = 0;
    uint32_t vaddr = (uint32_t)virtual_addr;
    uint32_t cnt = 0;

    if (pf == POOL_FLAG_KERNEL) {  // 内核虚拟内存池
        bit_idx = (vaddr - kernel_virtual_addr.addr_start) / PAGE_SIZE;
        while(cnt < pg_cnt) {
            bitmap_set(&kernel_virtual_addr.pool_bitmap, bit_idx + cnt, 0);
            cnt++;
        }
    } else {  // 用户虚拟内存池
        struct PCB_INFO* cur_thread_pcb = get_curthread_pcb();
        bit_idx = (vaddr - cur_thread_pcb->user_virtual_addr.addr_start) / PAGE_SIZE;
        while(cnt < pg_cnt) {
            bitmap_set(&cur_thread_pcb->user_virtual_addr.pool_bitmap, bit_idx + cnt, 0);
            cnt++;
        }
    }
}

/* 释放以虚拟地址 virtual_addr 为起始的 pg_cnt 个物理页框 */
void page_free(enum pool_flag pf, void* virtual_addr, uint32_t pg_cnt) {
    uint32_t phy_addr;
    uint32_t vaddr = (uint32_t)virtual_addr;
    uint32_t cnt = 0;
    ASSERT(pg_cnt >= 1 && vaddr % PAGE_SIZE == 0); 
    phy_addr = vaddr_to_phyaddr(vaddr);  // 获取虚拟地址vaddr对应的物理地址

    /* 确保待释放的物理内存在低端 1M + 1k 大小的页目录 + 1k 大小的页表地址范围外 */
    ASSERT((phy_addr % PAGE_SIZE) == 0 && phy_addr >= 0x102000);
    while (cnt < pg_cnt) {
        /* 释放物理内存 */
        phy_addr = vaddr_to_phyaddr(vaddr);
        phyaddr_free(phy_addr);

        /* 从页表中清除此虚拟地址所在的页表项pte */
        remove_pte_from_pagetable(vaddr);

        vaddr += PAGE_SIZE;
        cnt++;
    }
    /* 从虚拟地址池移除虚拟地址 */
    remove_virtual_addr(pf, virtual_addr, pg_cnt);
}

/* 释放内存 */
void sys_free(void* ptr) {
    if (ptr == NULL) {
        return;
    }
    enum pool_flag pf;
    struct memory_poll* mem_pool;

    if (get_curthread_pcb()->pgdir == NULL) {  // 内核线程
        pf = POOL_FLAG_KERNEL; 
        mem_pool = &kernel_memory_pool;
    } else {  // 用户进程
        pf = POOL_FLAG_USER;
        mem_pool = &user_memory_pool;
    }

    lock_acquire(&mem_pool->lock);
    struct mem_block* block = ptr;
    struct mem_arena* arena = get_arena_by_memblock(block);  // 把mem_block转换成arena, 获取元信息
    if ((arena->mem_block_pool_ptr == NULL) && (arena->isPage == true)) {  // 大于1024的内存，释放页
        page_free(pf, arena, arena->cnt); 
    } else {  // 小于等于 1024 的内存块
        /* 先将内存块回收到free_list */
        list_append(&arena->mem_block_pool_ptr->mem_block_list, &block->mem_elem);
        arena->cnt++;
        /* 再判断此arena中的内存块是否都是空闲,如果是就释放arena */
        if (arena->cnt == arena->mem_block_pool_ptr->mem_block_cnt) {
            uint32_t block_idx;
            for (block_idx = 0; block_idx < arena->mem_block_pool_ptr->mem_block_cnt; block_idx++) {
                struct mem_block* block = get_arena_memblock(arena, block_idx);
                ASSERT(elem_find(&arena->mem_block_pool_ptr->mem_block_list, &block->mem_elem));
                list_remove(&block->mem_elem);
            }
            page_free(pf, arena, 1);
        }
    }
    lock_release(&mem_pool->lock); 
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
    lock_init(&(kernel_memory_pool.lock));
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
    lock_init(&(user_memory_pool.lock));
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

/* 初始化堆内存块池子 */
void mem_block_pool_init(struct mem_block_pool* block_array) {				   
    uint32_t index;
    uint32_t block_size = 16;
    /* 初始化每个 mem_block_pool 描述符 */
    for (index = 0; index < MEMORY_POOL_COUNT; index++) {
        block_array[index].mem_block_size = block_size;
        /* 初始化arena中的内存块数量 */
        block_array[index].mem_block_cnt = (PAGE_SIZE - sizeof(struct mem_arena)) / block_size;	  
        list_init(&(block_array[index].mem_block_list));
        block_size *= 2;  // 下一个规格的内存块
   }
}

void memory_init()
{
    uint32_t memory_bytes = 32 * 1024 * 1024;  // 总共有32MB内存
    memory_pool_init(memory_bytes);
    mem_block_pool_init(kernel_memblock_pools);
}