#include "process.h"
#include "thread.h"
#include "string.h"
#include "boot.h"
#include "memory.h"
#include "debug.h"
#include "tss.h"
#include "console.h"
#include "interrupt.h"

extern void interrupt_exit(void);
/* 就绪线程队列 */
extern struct list g_readyThreadList;
/* 所有线程队列 */
extern struct list g_allThreadList;

void process_start(void* _filename) {
    console_put_str("process_start\n");
    void* func = _filename;
    struct PCB_INFO* cur_pcb = get_curthread_pcb();
    /* 获取中断栈指针 */
    cur_pcb->self_kstack += sizeof(struct THREAD_STACK);
    struct INTR_STACK* intr_stack = (struct INTR_STACK*)cur_pcb->self_kstack;
    memset(intr_stack, 0, sizeof(struct INTR_STACK));
    intr_stack->ds = SL_USRDATA;
    intr_stack->es = SL_USRDATA;
    intr_stack->fs = SL_USRDATA;
    intr_stack->eip = func;
    intr_stack->cs = SL_USRCODE;
    struct EFLAGS eflags = { 0 };
    eflags.IOPL = 0;
    eflags.MBS = 1;
    eflags.IF = 1;
    uint32_t* p_eflags = (uint32_t*)(&eflags);
    intr_stack->eflags = *p_eflags;
    intr_stack->esp = (void*)((uint32_t)bind_vaddr_with_mempool(POOL_FLAG_USER, USER_STACK_VADDR) + PAGE_SIZE);
    intr_stack->ss = SL_USRDATA;
    asm volatile("movl %0, %%esp; jmp interrupt_exit" : : "g" (intr_stack) : "memory");
    console_put_str("process_end\n");
}

/* 激活页表 */
void process_pgdir_active(struct PCB_INFO* p_pcb) {
    console_put_str("process_pgdir_active_start\n");
    /********************************************************
    * 执行此函数时,当前任务可能是线程。
    * 之所以对线程也要重新安装页表, 原因是上一次被调度的可能是进程,
    * 否则不恢复页表的话,线程就会使用进程的页表了。
    ********************************************************/

    /* 若为内核线程,需要重新填充页表为0x100000 */
    uint32_t pgdir_phyaddr = 0x100000;  // 默认为内核的页目录物理地址,也就是内核线程所用的页目录表
    if (p_pcb->pgdir != NULL)	{       // 用户态进程有自己的页目录表
        pgdir_phyaddr = vaddr_to_phyaddr((uint32_t)p_pcb->pgdir);
    }

    console_put_str("pgdir_phyaddr = ");
    console_put_int(pgdir_phyaddr);
    console_put_char('\n');

    /* 更新页目录寄存器cr3, 使新页表生效 */
    asm volatile ("movl %0, %%cr3" : : "r" (pgdir_phyaddr) : "memory");
    console_put_str("process_pgdir_active_done\n");
}

void process_active(struct PCB_INFO* p_pcb) {
    console_put_str("process_active start\n");
    ASSERT(p_pcb != NULL);
    process_pgdir_active(p_pcb);
    if (p_pcb->pgdir != NULL) {
        tss_update_esp0(p_pcb);
    }
    console_put_str("process_active done\n");
}

/* 创建进程的页目录表 */
uint32_t* process_create_pgdir() {
    uint32_t* pg_vaddr = get_kernel_pages(1);
    if (pg_vaddr == NULL) {
        console_put_str("process_create_pgdir: get_kernel_page failed!\n");
        return NULL;
    }
    /* 拷贝内核页目录表 第 768 项 ~ 第 1023 项，即：内核区间页目录表 */
    memcpy((void*)((uint32_t)pg_vaddr + 0x300 * 4), (void*)(0xFFFFF000 + 0x300 * 4), 256 * 4);
    /* 页目录表最后一项指向进程页目录表的起始物理页 */
    uint32_t pg_phyaddr = vaddr_to_phyaddr((uint32_t)pg_vaddr);
    struct page_directory_entry* pde = (struct page_directory_entry*)(&pg_phyaddr);
    pde->P = 1;
    pde->RW = 1;
    pde->US = 1;
    pg_vaddr[1023] = *((uint32_t*)pde);
    console_put_str("process_create_pgdir: pde = ");
    console_put_int(pg_vaddr[1023]);
    console_put_char('\n');
    console_put_str("process_create_pgdir: pg_vaddr = ");
    console_put_int((uint32_t)pg_vaddr);
    console_put_char('\n');
    return pg_vaddr;
}

/* 用户程序的 bitmap 创建 */
void process_bitmap_create(struct PCB_INFO* p_pcb) {
    p_pcb->user_virtual_addr.addr_start = USER_VADDR_START;
    // 用户程序页表数
    uint32_t pgcnt = (0xC0000000 - USER_VADDR_START) / PAGE_SIZE;
    // 用户程序页表 bitmap 的字节数
    uint32_t bitmap_byte_cnt = pgcnt / 8;
    // 用户程序页表 bitmap 所需页表数： 向上取整
    uint32_t bitmap_pgcnt = (bitmap_byte_cnt + PAGE_SIZE - 1) / PAGE_SIZE;
    p_pcb->user_virtual_addr.pool_bitmap.bits = get_kernel_pages(bitmap_pgcnt);
    p_pcb->user_virtual_addr.pool_bitmap.bytes_num = bitmap_byte_cnt;
    bitmap_init(&(p_pcb->user_virtual_addr.pool_bitmap));
}

/* 创建用户进程 */
void process_execute(void* filename, char* name) { 
    /* pcb内核的数据结构,由内核来维护进程信息,因此要在内核内存池中申请 */
    struct PCB_INFO* process_pcb = get_kernel_pages(1);
    init_thread_pcb(process_pcb, name, USER_DEFAULT_PRIORITY);
    process_bitmap_create(process_pcb);
    init_thread_stack(process_pcb, process_start, filename);
    process_pcb->pgdir = process_create_pgdir();

    enum interrupt_status old_status = get_interrupt_status();
    interrupt_disable();
    ASSERT(!elem_find(&g_readyThreadList, &process_pcb->general_tag));
    list_append(&g_readyThreadList, &process_pcb->general_tag);

    ASSERT(!elem_find(&g_allThreadList, &process_pcb->all_list_tag));
    list_append(&g_allThreadList, &process_pcb->all_list_tag);
    set_interrupt_status(old_status);
}