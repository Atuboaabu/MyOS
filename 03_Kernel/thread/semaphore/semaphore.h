#ifndef _LIB_KERNEL_SEMAPHORE_H_
#define _LIB_KERNEL_SEMAPHORE_H_

#include "stdint.h"
#include "list.h"

struct semaphore {
    uint32_t value;         // 信号量的值
    struct list waiters_list;  // 等待信号量的队列
};

/* 信号量的初始化 */
void semaphore_init(struct semaphore *sema, uint32_t value);
/* 信号量 P 操作 */
void semaphore_P(struct semaphore *sema);
/* 信号量 V 操作 */
void semaphore_V(struct semaphore *sema);

#endif