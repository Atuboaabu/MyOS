#ifndef _LIB_KERNEL_LOCK_H_
#define _LIB_KERNEL_LOCK_H_

#include "stdint.h"
#include "semaphore.h"

/* 锁结构 */
struct lock {
    struct PCB_INFO* holder;
    struct semaphore semaphore;
    uint32_t holder_request_num;
};

/* 初始化锁 */
void lock_init(struct lock* p_lock);
/* 申请锁 */
void lock_acquire(struct lock* p_lock);
/* 释放锁 */
void lock_release(struct lock* p_lock);

#endif
