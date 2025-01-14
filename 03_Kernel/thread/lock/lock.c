#include "lock.h"
#include "thread.h"
#include "debug.h"
#include "print.h"

void lock_init(struct lock* p_lock) {
    p_lock->holder = NULL;
    // 初始化信号量为二元信号量，用作互斥判断
    semaphore_init(&(p_lock->semaphore), 1);
    p_lock->holder_request_num = 0;
}

void lock_acquire(struct lock* p_lock) {
    if (p_lock->holder != get_curthread_pcb()) {
        p_lock->holder = get_curthread_pcb();
        p_lock->holder_request_num = 1;
        semaphore_P(&(p_lock->semaphore));
    } else {
        p_lock->holder_request_num++;
    }
}

void lock_release(struct lock* p_lock) {
    if (p_lock->holder_request_num > 1) {
        p_lock->holder_request_num--;
        return;
    }
    p_lock->holder = NULL;
    p_lock->holder_request_num = 0;
    semaphore_V(&(p_lock->semaphore));
}