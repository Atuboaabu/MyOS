#include "condition_variable.h"
#include "lock.h"
#include "list.h"
#include "thread.h"

void condition_init(struct condition_variable* p_cond) {
    list_init(&(p_cond->waiters_list));
}

void condition_wait(struct condition_variable* p_cond, struct lock* p_lock) {
    struct PCB_INFO* cur_pcb = get_curthread_pcb();
    if (elem_find(&(p_cond->waiters_list), &(cur_pcb->general_tag))) {  // 如果当前线程在等待队列，则不处理
        put_str("\ncondition_wait error! pcb in waiters list!\n");
    }
    list_push(&(p_cond->waiters_list), &(cur_pcb->general_tag));
    lock_release(p_lock);  // 释放锁，让其他线程可以进入临界区
    thread_block(TASK_BLOCKED);
    lock_acquire(p_lock);  // 线程恢复后在此处，进入处理前需要再次获取锁
}

void condition_notify(struct condition_variable* p_cond) {
    if (!list_empty(&(p_cond->waiters_list))) {
        struct PCB_INFO* blocked_thread_pcb =
            GET_ENTRYPTR_FROM_LISTTAG(struct PCB_INFO, general_tag, list_pop(&(p_cond->waiters_list)));
        thread_unblock(blocked_thread_pcb);
    }
}

void condition_notifyall(struct condition_variable* p_cond) {
    while (!list_empty(&(p_cond->waiters_list))) {
        condition_notify(p_cond);
    }
}