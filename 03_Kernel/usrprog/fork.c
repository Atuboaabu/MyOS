#include "fork.h"
#include "stdint.h"
#include "thread.h"
#include "string.h"
#include "memory.h"
#include "process.h"
#include "list.h"
#include "file.h"

extern void interrupt_exit(void);
extern struct file g_fielTable[MAX_FILES];
extern struct list g_readyThreadList;
extern struct list g_allThreadList;

static int32_t copy_pcb_info(struct PCB_INFO* child_thread_pcb, struct PCB_INFO* parent_thread_pcb) {
    /* 拷贝pcb中所有信息 */
    memcpy(child_thread_pcb, parent_thread_pcb, PAGE_SIZE);
    /* 差异部分修改 */
    child_thread_pcb->pid = fork_pid();
    child_thread_pcb->elapsed_ticks = 0;
    child_thread_pcb->status = TASK_READY;
    child_thread_pcb->ticks = child_thread_pcb->priority;
    child_thread_pcb->parent_pid = parent_thread_pcb->pid;
    /* 链表 tag 复位 */
    child_thread_pcb->general_tag.prev = NULL;
    child_thread_pcb->general_tag.next = NULL;
    child_thread_pcb->all_list_tag.prev = NULL;
    child_thread_pcb->all_list_tag.next = NULL;
    /* 用户内存池重新初始化 */
    mem_block_pool_init(child_thread_pcb->user_memblock_pools);
    /* 虚拟地址池位图创建 */
    process_bitmap_create(child_thread_pcb);
    memcpy(child_thread_pcb->user_virtual_addr.pool_bitmap.bits,
           parent_thread_pcb->user_virtual_addr.pool_bitmap.bits,
           parent_thread_pcb->user_virtual_addr.pool_bitmap.bytes_num);
    return 0;
}

static int32_t copy_process_body(struct PCB_INFO* child_thread_pcb, struct PCB_INFO* parent_thread_pcb) {
    uint8_t* parent_vaddr_bmp = parent_thread_pcb->user_virtual_addr.pool_bitmap.bits;
    uint32_t parent_vaddr_bmp_bytesnum = parent_thread_pcb->user_virtual_addr.pool_bitmap.bytes_num;
    uint32_t vaddr_start = parent_thread_pcb->user_virtual_addr.addr_start;
    void* buf_page = get_kernel_pages(1);  // 申请一个内核页用作 buf
    if (buf_page == NULL) {
        return -1;
    }
    
    for (uint32_t i = 0; i < parent_vaddr_bmp_bytesnum; i++) {
        if (parent_vaddr_bmp[i] == 0) {
            continue;
        }
        for (int j = 0; j < 8; j++) {
            if (((1 << j) & parent_vaddr_bmp[i])) {
                uint32_t vaddr = (i * 8 + j) * PAGE_SIZE + vaddr_start;
                /* 将父进程对应虚拟地址页的信息拷贝到内核buf */
                memcpy(buf_page, (void*)vaddr, PAGE_SIZE);
                /* 切换页表为子进程的页表 */
                process_pgdir_active(child_thread_pcb);
                /* 获取一页内存 */
                get_a_page_without_set_vaddrbmp(POOL_FLAG_USER, vaddr);
                /* 将内核buf中的数据拷贝到子进程 */
                memcpy((void*)vaddr, buf_page, PAGE_SIZE);
                /* 切换页表为父进程的页表 */
                process_pgdir_active(parent_thread_pcb);
            }
        }
    }
    page_free(POOL_FLAG_KERNEL, buf_page, 1);
    return 0;
}

/* 子进程栈构建 */
static int32_t build_child_process_stack(struct PCB_INFO* child_thread_pcb, struct PCB_INFO* parent_thread_pcb) {
    /* 修改子进程的返回值为0 */
    struct INTR_STACK* intr_stack = (struct INTR_STACK*)((uint32_t)child_thread_pcb + PAGE_SIZE - sizeof(struct INTR_STACK));
    intr_stack->eax = 0;
    /* 修改线程栈信息 */
    uint32_t* ret_addr = (uint32_t*)intr_stack - 1;
    uint32_t* esi_addr = (uint32_t*)intr_stack - 2;
    uint32_t* edi_addr = (uint32_t*)intr_stack - 3;
    uint32_t* ebx_addr = (uint32_t*)intr_stack - 4;
    uint32_t* ebp_addr = (uint32_t*)intr_stack - 5;
    *ret_addr = (uint32_t)interrupt_exit;
    *esi_addr = 0;
    *edi_addr = 0;
    *ebx_addr = 0;
    *ebp_addr = 0;
    child_thread_pcb->self_kstack = ebp_addr;
    return 0;
}

static void update_inode_open_cnt(struct PCB_INFO* thread_pcb) {
    int32_t local_fd = 3;
    int32_t global_fd = 0;
    while (local_fd < PROCESS_MAX_FILE_NUM) {
        global_fd = thread_pcb->fd_table[local_fd];
        if (global_fd != -1) {
            g_fielTable[global_fd].fd_inode->i_open_cnts++;
        }
        local_fd++;
    }
}

static int32_t copy_process_info(struct PCB_INFO* child_thread_pcb, struct PCB_INFO* parent_thread_pcb) {
    /* 拷贝 pcb 信息 */
    int32_t ret = copy_pcb_info(child_thread_pcb, parent_thread_pcb);
    if (ret != 0) {
        return -1;
    }
    /* 子进程页表创建 */
    child_thread_pcb->pgdir = process_create_pgdir();
    if (child_thread_pcb->pgdir == NULL) {
        return -1;
    }
    /* 拷贝函数体部分 */
    ret = copy_process_body(child_thread_pcb, parent_thread_pcb);
    if (ret != 0) {
        return -1;
    }
    /* 构建子进程运行栈信息 */
    build_child_process_stack(child_thread_pcb, parent_thread_pcb);
    /* 更新文件inode的打开次数 */
    update_inode_open_cnt(child_thread_pcb);
    return 0;
}


pid_t sys_fork() {
    struct PCB_INFO* parent_thread_pcb = get_curthread_pcb();
    struct PCB_INFO* child_thread_pcb = get_kernel_pages(1);
    if (child_thread_pcb == NULL) {
        return -1;
    }

    int32_t ret = copy_process_info(child_thread_pcb, parent_thread_pcb);
    if (ret != 0) {
        return -1;
    }

    /* 加入就绪线程队列 */
    list_append(&g_readyThreadList, &child_thread_pcb->general_tag);
    /* 加入全部线程队列 */
    list_append(&g_allThreadList, &child_thread_pcb->all_list_tag);
    return child_thread_pcb->pid;
}