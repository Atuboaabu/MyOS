#include "semaphore.h"
#include "thread.h"
#include "print.h"
#include "interrupt.h"
#include "debug.h"

void semaphore_init(struct semaphore *p_sema, uint32_t value) {
    p_sema->value = value;
    list_init(&(p_sema->waiters_list));
}

void semaphore_P(struct semaphore *p_sema) {
    enum interrupt_status old_status = get_interrupt_status();
    interrupt_disable();
    // 为什么用 while ：线程唤醒后信号量的值可能还为0，需要再次阻塞
    while (p_sema->value == 0) {
        struct PCB_INFO* cur_pcb = get_curthread_pcb();
        if (elem_find(&(p_sema->waiters_list), &(cur_pcb->general_tag))) {  // 如果当前线程在等待队列，则不处理
            put_str("\nsemaphore_P error! pcb in waiters list!\n");
        }
        list_push(&(p_sema->waiters_list), &(cur_pcb->general_tag));
        thread_block(TASK_BLOCKED);
    }
    // 线程唤醒后在这，此时的信号值为1，需要在此处减1
    p_sema->value--;
    set_interrupt_status(old_status);
}

void semaphore_V(struct semaphore *p_sema) {
    enum interrupt_status old_status = get_interrupt_status();
    interrupt_disable();
    p_sema->value++;
    if (!list_empty(&(p_sema->waiters_list))) {
        struct PCB_INFO* blocked_thread_pcb =
            GET_ENTRYPTR_FROM_LISTTAG(struct PCB_INFO, general_tag, list_pop(&(p_sema->waiters_list)));
        thread_unblock(blocked_thread_pcb);
    }
    set_interrupt_status(old_status);
}