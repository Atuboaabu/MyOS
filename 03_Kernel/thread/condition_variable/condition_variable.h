#ifndef _LIB_KERNEL_CONDITION_VARIABLE_H_
#define _LIB_KERNEL_CONDITION_VARIABLE_H_

#include "stdint.h"
#include "list.h"
#include "lock.h"

struct condition_variable {
    struct list waiters_list;
};

/* 初始化条件变量 */
void condition_init(struct condition_variable* p_cond);
/* 条件变量满足，阻塞当前线程 */
void condition_wait(struct condition_variable* p_cond, struct lock* p_lock);
/* 恢复一个条件阻塞的线程 */
void condition_notify(struct condition_variable* p_cond);
/* 恢复所有条件阻塞的线程 */
void condition_notifyall(struct condition_variable* p_cond);

#endif
