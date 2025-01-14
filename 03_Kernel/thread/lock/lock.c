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
    semaphore_P(&(p_lock->semaphore));
}

void lock_release(struct lock* p_lock) {
    semaphore_V(&(p_lock->semaphore));
}