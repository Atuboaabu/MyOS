#ifndef _USRPROG_FORK_H_
#define _USRPROG_FORK_H_

#include "thread.h"

/* fork子进程：成功后父进程返回子进程 pid，子进程返回0；失败返回 -1 */
pid_t sys_fork();

#endif
