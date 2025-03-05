#include "system.h"
#include "stdint.h"
#include "list.h"
#include "thread.h"
#include "memory.h"

extern struct list g_allThreadList;

/* 查找状态为 TASK_HANGING 的任务 */
static bool find_hanging_chil(struct list_elem* pelem, int32_t ppid) {
    struct PCB_INFO* pcb = GET_ENTRYPTR_FROM_LISTTAG(struct PCB_INFO, all_list_tag, pelem);
    if ((pcb->parent_pid == ppid) && (pcb->status == TASK_HANGING)) {
        return true;
    }
    return false;
}

/* 查找父进程 pid 为 ppid 的进程 */
static bool find_child(struct list_elem* pelem, int32_t ppid) {
    struct PCB_INFO* pthread = GET_ENTRYPTR_FROM_LISTTAG(struct PCB_INFO, all_list_tag, pelem);
    if (pthread->parent_pid == ppid) {
        return true;
    }
    return false;
}

/* 等待子进程调用 exit, 将子进程的退出状态保存到status指向的变量: 成功则返回子进程的pid, 失败则返回 -1 */
pid_t sys_wait(int32_t* status) {
    struct PCB_INFO* cur_thread_pcb = get_curthread_pcb();
    while(1) {
        /* 优先处理已经是挂起状态的任务 */
        struct list_elem* child_elem = list_traversal(&g_allThreadList, find_hanging_chil, cur_thread_pcb->pid);
        /* 若有挂起的子进程 */
        if (child_elem != NULL) {
            struct PCB_INFO* child_thread_pcb = GET_ENTRYPTR_FROM_LISTTAG(struct PCB_INFO, all_list_tag, child_elem);
            *status = child_thread_pcb->exit_status; 
            uint16_t child_pid = child_thread_pcb->pid;
            /* 释放进程 */
            thread_exit(child_thread_pcb, false);
            return child_pid;
        } 

        /* 判断是否有子进程 */
        child_elem = list_traversal(&g_allThreadList, find_child, cur_thread_pcb->pid);
        if (child_elem == NULL) {  // 无子进程
            return -1;
        } else {
            /* 若子进程还未运行完， 将自己挂起, 直到子进程在执行 exit 时将自己唤醒 */
            thread_block(TASK_WAITING); 
        }
    }
}

/* 将 pid 对应进程的 1 个子进程过继给 init */
static bool init_adopt_a_child(struct list_elem* pelem, int32_t pid) {
    struct PCB_INFO* pthread = GET_ENTRYPTR_FROM_LISTTAG(struct PCB_INFO, all_list_tag, pelem);
    if (pthread->parent_pid == pid) {
        pthread->parent_pid = 1;
    }
    return false;
}

/* 释放用户进程资源 */
 static void release_process_resource(struct PCB_INFO* pcb) {
    uint32_t* pgdir_vaddr = pcb->pgdir;
    uint16_t user_pde_cnt = 768;
    uint16_t pde_idx = 0;
    uint32_t pde = 0;
    uint32_t* pde_ptr = NULL;
    uint16_t user_pte_cnt = 1024, pte_idx = 0;
    uint32_t pte = 0;
    uint32_t* pte_ptr = NULL;
    uint32_t* pte_vaddr = NULL;
    uint32_t pg_phy_addr = 0;

    /* 回收页表中用户空间的页框 */
    while (pde_idx < user_pde_cnt) {
        pde_ptr = pgdir_vaddr + pde_idx;
        pde = *pde_ptr;
        if (pde & 0x00000001) {  // pde存在
            pte_vaddr = get_pte_ptr(pde_idx * 0x400000);
            pte_idx = 0;
            while (pte_idx < user_pte_cnt) {
                pte_ptr = pte_vaddr + pte_idx;
                pte = *pte_ptr;
                if (pte & 0x00000001) {  // pte 存在
                    pg_phy_addr = pte & 0xfffff000;
                    free_a_phy_page(pg_phy_addr);
                }
                pte_idx++;
            }
            pg_phy_addr = pde & 0xfffff000;
            free_a_phy_page(pg_phy_addr);
        }
        pde_idx++;
    }

    /* 回收用户虚拟地址池所占的物理内存*/
    uint32_t bitmap_pg_cnt = (pcb->user_virtual_addr.pool_bitmap.bytes_num) / PAGE_SIZE;
    uint8_t* bits_addr = pcb->user_virtual_addr.pool_bitmap.bits;
    page_free(POOL_FLAG_KERNEL, bits_addr, bitmap_pg_cnt);
 
    /* 关闭进程打开的文件 */
    uint8_t local_fd = 3;
    while(local_fd < PROCESS_MAX_FILE_NUM) {
        if (pcb->fd_table[local_fd] != -1) {
            sys_close(local_fd);
        }
        local_fd++;
    }
}

/* 子进程用来结束调用 */
void sys_exit(int32_t status) {
    struct PCB_INFO* cur_thread_pcb = get_curthread_pcb();
    cur_thread_pcb->exit_status = status; 
    if (cur_thread_pcb->parent_pid == -1) {
        PANIC("sys_exit: cur_thread_pcb->parent_pid is -1\n");
    }
    /* 将进程的所有子进程都过继给init */
    list_traversal(&g_allThreadList, init_adopt_a_child, cur_thread_pcb->pid);
    /* 回收进程cur_thread_pcb的资源 */
    release_process_resource(cur_thread_pcb); 
    /* 如果父进程正在等待子进程退出,将父进程唤醒 */
    struct PCB_INFO* parent_thread_pcb = get_thread_PCB(cur_thread_pcb->parent_pid);
    if (parent_thread_pcb->status == TASK_WAITING) {
        thread_unblock(parent_thread_pcb);
    }
    /* 将自己挂起, 等待父进程获取其status, 并回收其pcb */
    thread_block(TASK_HANGING);
}