#include "thread.h"
#include "list.h"
#include "memory.h"
#include "interrupt.h"
#include "debug.h"
#include "process.h"

#define PG_SIZE (4096)
/* 主线程PCB */
static struct PCB_INFO* s_mainThreadPCB;
/* idle线程PCB */
static struct PCB_INFO* s_idleThreadPCB;
/* 就绪线程队列 */
struct list g_readyThreadList;
/* 所有线程队列 */
struct list g_allThreadList;
/* 线程结点 */
struct list_elem* g_curThreadTag;
/* pid申请的互斥锁 */
static struct lock pid_lock;
/* pid 池 */
static struct pid_pool {
    struct bitmap pid_bitmap;  // pid位图
    uint32_t pid_start;        // 起始pid
    struct lock pid_lock;      // 分配pid锁
} s_pidPool;
/* pid 的位图, 最大支持 128 * 8 = 1024 个pid */
static uint8_t s_pidBitmapBits[128] = { 0 };
/* 跳转执行函数：由汇编语言实现 */
extern void switch_to(struct PCB_INFO* cur_pcb, struct PCB_INFO* next_pcb);

/**************** 系统空闲的线程 *******************/
static void idle_func(void* arg) {
    while(1) {
        thread_block(TASK_BLOCKED);     
        //执行hlt时必须要保证目前处在开中断的情况下
        asm volatile("sti; hlt" : : : "memory");
    }
}

/**************** 线程信息获取函数 *******************/
/* 初始化pid池 */
static void pid_pool_init(void) { 
    s_pidPool.pid_start = 1;
    s_pidPool.pid_bitmap.bits = s_pidBitmapBits;
    s_pidPool.pid_bitmap.bytes_num = 128;
    bitmap_init(&s_pidPool.pid_bitmap);
    lock_init(&s_pidPool.pid_lock);
}
/* 申请 pid */
static pid_t allocate_pid() {
    lock_acquire(&s_pidPool.pid_lock);
    int32_t bit_idx = bitmap_apply(&s_pidPool.pid_bitmap, 1);
    bitmap_set(&s_pidPool.pid_bitmap, bit_idx, 1);
    lock_release(&s_pidPool.pid_lock);
    return (bit_idx + s_pidPool.pid_start);
}
/* 释放pid */
void release_pid(pid_t pid) {
    lock_acquire(&s_pidPool.pid_lock);
    int32_t bit_idx = pid - s_pidPool.pid_start;
    bitmap_set(&s_pidPool.pid_bitmap, bit_idx, 0);
    lock_release(&s_pidPool.pid_lock);
}
/* 为 fork 进程提供 pid 获取函数 */
pid_t fork_pid() {
    return allocate_pid();
}

/* 获取当前进程/线程 pid */
uint32_t sys_getpid() {
    return get_curthread_pcb()->pid;
}

/**************** 线程调度相关函数 *******************/
/* 获取当前线程pcb指针 */
struct PCB_INFO* get_curthread_pcb() {
    uint32_t esp; 
    asm ("mov %%esp, %0" : "=g" (esp));
    /* 取esp整数部分即pcb页起始地址 */
    return (struct PCB_INFO*)(esp & 0xfffff000);
}

void thread_schedule() {
    ASSERT(get_interrupt_status() == INTERRUPT_DISABLE);
    struct PCB_INFO* cur_thread_pcb = get_curthread_pcb(); 
    if (cur_thread_pcb->status == TASK_RUNNING) { // 若此线程只是cpu时间片到了,将其加入到就绪队列尾
        // ASSERT(!elem_find(&g_readyThreadList, &cur_thread_pcb->general_tag));
        list_append(&g_readyThreadList, &cur_thread_pcb->general_tag);
        cur_thread_pcb->ticks = cur_thread_pcb->priority;   // 重置 ticks 为其 priority
        cur_thread_pcb->status = TASK_READY;
    } else { 
        /* 若此线程需要某事件发生后才能继续上cpu运行,
        不需要将其加入队列, 因为当前线程不在就绪队列中。*/
    }

    if (list_empty(&g_readyThreadList)) {  // 如果就绪队列为空，则运行 idle 线程
        thread_unblock(s_idleThreadPCB);
    }
    ASSERT(!list_empty(&g_readyThreadList));

    g_curThreadTag = NULL;	  // thread_tag清空
    g_curThreadTag = list_pop(&g_readyThreadList);  // 取出队列的第一个线程跳转执行
    struct PCB_INFO* next_thread_pcb = GET_ENTRYPTR_FROM_LISTTAG(struct PCB_INFO, general_tag, g_curThreadTag);
    next_thread_pcb->status = TASK_RUNNING;
    process_active(next_thread_pcb);
    switch_to(cur_thread_pcb, next_thread_pcb);
}

/**************** 线程阻塞相关函数 *******************/
/* 当前线程将自己阻塞 */
void thread_block(enum TASK_STATUS status) {
    /* stat取值为 TASK_BLOCKED, TASK_WAITING, TASK_HANGING, 也就是只有这三种状态才不会被调度*/
    ASSERT(((status == TASK_BLOCKED) || (status == TASK_WAITING) || (status == TASK_HANGING)));
    enum interrupt_status old_status = get_interrupt_status();
    interrupt_disable();
    struct PCB_INFO* cur_thread_pcb = get_curthread_pcb();
    cur_thread_pcb->status = status;  // 置其状态为status
    thread_schedule();                // 将当前线程换下处理器
    /* 待当前线程被解除阻塞后才继续运行下面的 set_interrupt_status */
    set_interrupt_status(old_status);
}

/* 将线程解除阻塞 */
void thread_unblock(struct PCB_INFO* pthread) {
    enum interrupt_status old_status = get_interrupt_status();
    interrupt_disable();
    ASSERT(((pthread->status == TASK_BLOCKED) || (pthread->status == TASK_WAITING) || (pthread->status == TASK_HANGING)));
    if (pthread->status != TASK_READY) {
        if (elem_find(&g_readyThreadList, &pthread->general_tag)) {
            return;
        }
        pthread->status = TASK_READY;
        list_push(&g_readyThreadList, &pthread->general_tag);    // 放到队列的最前面,使其尽快得到调度
    } 
    set_interrupt_status(old_status);
}

/* 线程挂起，主动让出cpu，换其它线程运行 */
void thread_yield(void) {
    struct PCB_INFO* cur_thread_PCB = get_curthread_pcb();
    enum interrupt_status old_status = get_interrupt_status();
    interrupt_disable();
    ASSERT(!elem_find(&g_readyThreadList, &cur_thread_PCB->general_tag));
    list_append(&g_readyThreadList, &cur_thread_PCB->general_tag);
    cur_thread_PCB->status = TASK_READY;
    thread_schedule();
    set_interrupt_status(old_status);
}

/**************** 线程创建相关函数 *******************/
/* 初始化线程基本信息 */
void init_thread_pcb(struct PCB_INFO* thread_pcb, char* name, int prio) {
    memset(thread_pcb, 0, sizeof(*thread_pcb));
    strcpy(thread_pcb->name, name);
    thread_pcb->pid = allocate_pid();

    if (thread_pcb == s_mainThreadPCB) {
        /* 由于把main函数也封装成一个线程, 并且它一直是运行的, 故将其直接设为 TASK_RUNNING */
        thread_pcb->status = TASK_RUNNING;
    } else {
        thread_pcb->status = TASK_READY;
    }

    /* self_kstack 是线程自己在内核态下使用的栈顶地址 */
    thread_pcb->self_kstack = (uint32_t*)((uint32_t)thread_pcb + PG_SIZE);
    thread_pcb->priority = prio;
    thread_pcb->ticks = prio;
    thread_pcb->elapsed_ticks = 0;
    thread_pcb->pgdir = NULL;
    /* 文件描述符数组初始化 */
    thread_pcb->fd_table[0] = 0;  // 标准输入
    thread_pcb->fd_table[1] = 1;  // 标准输出
    thread_pcb->fd_table[2] = 2;  // 标准错误
    thread_pcb->fd_table[3] = -1;
    thread_pcb->fd_table[4] = -1;
    thread_pcb->fd_table[5] = -1;
    thread_pcb->fd_table[6] = -1;
    thread_pcb->fd_table[7] = -1;

    thread_pcb->cwd_inode_num = 0;         // 默认为根目录 inode
    thread_pcb->stack_magic = 0x19870916;  // 自定义的魔数
}
/* 由 thread_func 去执行 start_routine(arg) */
static void thread_running(thread_func start_routine, void* arg) {
    interrupt_enable();
    start_routine(arg);
}
/* 初始化线程栈 THREAD_STACK, 将待执行的函数和参数放到 THREAD_STACK 中相应的位置 */
void init_thread_stack(struct PCB_INFO* thread_pcb, thread_func start_routine, void* arg) {
    /* 预留中断栈空间 */
    thread_pcb->self_kstack -= sizeof(struct INTR_STACK);
    /* 预留线程栈空间 */
    thread_pcb->self_kstack -= sizeof(struct THREAD_STACK);
    struct THREAD_STACK* kthread_stack = (struct THREAD_STACK*)thread_pcb->self_kstack;
    kthread_stack->eip = thread_running;
    kthread_stack->function = start_routine;
    kthread_stack->func_arg = arg;
    kthread_stack->ebp = 0;
    kthread_stack->ebx = 0;
    kthread_stack->esi = 0;
    kthread_stack->edi = 0;
}
/* 创建一优先级为prio的线程, 线程名为name, 线程所执行的函数是 function(func_arg) */
struct PCB_INFO* thread_create(char* name, int prio, thread_func start_routine, void* arg) {
    /* 从内核申请PCB对应的内存 */
    struct PCB_INFO* pcb = get_kernel_pages(1);

    init_thread_pcb(pcb, name, prio);
    init_thread_stack(pcb, start_routine, arg);
    /* 加入就绪线程队列 */
    // ASSERT(!elem_find(&g_readyThreadList, &pcb->general_tag));
    list_append(&g_readyThreadList, &pcb->general_tag);
    /* 加入全部线程队列 */
    // ASSERT(!elem_find(&g_allThreadList, &pcb->all_list_tag));
    list_append(&g_allThreadList, &pcb->all_list_tag);
    return pcb;
}
/* 回收 thread_over 的 pcb 和页表, 并将其从调度队列中去除 */
void thread_exit(struct PCB_INFO* thread_over, bool need_schedule) {
    /* 要保证 schedule 在关中断情况下调用 */
    interrupt_disable();
    thread_over->status = TASK_DEAD;
    /* 将线程pcb从就绪队列中移除 */
    if (elem_find(&g_readyThreadList, &thread_over->general_tag)) {
        list_remove(&thread_over->general_tag);
    }
    /* 释放页表 */
    if (thread_over->pgdir) {
        page_free(POOL_FLAG_KERNEL, thread_over->pgdir, 1);
    }
    /* 从线程队列中移除 */
    list_remove(&thread_over->all_list_tag);
    /* 释放线程 pcb 页 */
    if (thread_over != s_mainThreadPCB) {
        page_free(POOL_FLAG_KERNEL, thread_over, 1);
    }
    /* 释放pid */
    release_pid(thread_over->pid);
    /* 如需调度，则调度 */
    if (need_schedule) {
        thread_schedule();
        put_str("thread_exit: should not be here\n");
    }
}

/* 比对任务的pid */
static bool pid_check(struct list_elem* pelem, pid_t pid) {
    struct PCB_INFO* pthread = GET_ENTRYPTR_FROM_LISTTAG(struct PCB_INFO, all_list_tag, pelem);
    if (pthread->pid == pid) {
        return true;
    }
    return false;
}
 
/* 根据 pid 找 pcb: 若找到则返回该 pcb地址, 否则返回 NULL */
struct PCB_INFO* get_thread_PCB(pid_t pid) {
    struct list_elem* pelem = list_traversal(&g_allThreadList, pid_check, pid);
    if (pelem == NULL) {
        return NULL;
    }
    struct PCB_INFO* pcb = GET_ENTRYPTR_FROM_LISTTAG(struct PCB_INFO, all_list_tag, pelem);
    return pcb;
}

/**************** 内核进程 main 转换为线程相关操作  *******************/
/* 将kernel中的main函数完善为主线程 */
static void make_kernelmain_to_thread(void) {
    /* 因为main线程早已运行,咱们在loader.S中进入内核时的 mov esp,0xc009f000,
    *  就是为其预留了pcb,地址为0xc009e000,因此不需要通过get_kernel_page另分配一页 */
    s_mainThreadPCB = get_curthread_pcb();
    put_str("\ns_mainThreadPCB: ");
    put_int((uint32_t)s_mainThreadPCB);
    put_char('\n');
    init_thread_pcb(s_mainThreadPCB, "main", 20);
    /* main函数是当前线程, 当前线程不在 g_readyThreadList 中,
    * 所以只将其加在 g_allThreadList 中. */
    ASSERT(!elem_find(&g_allThreadList, &s_mainThreadPCB->all_list_tag));
    list_append(&g_allThreadList, &s_mainThreadPCB->all_list_tag);
}

/**************** 初始化 *******************/
void thread_init(void) {
    put_str("thread_init start\n");
    list_init(&g_readyThreadList);
    list_init(&g_allThreadList);
    lock_init(&pid_lock);
    pid_pool_init();
    /* 将当前main函数创建为线程 */
    make_kernelmain_to_thread();
    s_idleThreadPCB = thread_create("idle_thread", 10, idle_func, NULL);
    put_str("thread_init done\n");
}
