#ifndef _USRPROG_SYSTEM_H_
#define _USRPROG_SYSTEM_H_

#include "stdint.h"
#include "thread.h"

/* 子进程用来结束调用 */
void sys_exit(int32_t status);
/* 等待子进程调用 exit, 将子进程的退出状态保存到status指向的变量: 成功则返回子进程的pid, 失败则返回 -1 */
pid_t sys_wait(int32_t* status);

#endif
