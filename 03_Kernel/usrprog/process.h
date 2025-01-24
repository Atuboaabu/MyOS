#ifndef _USRPROG_PROCESS_H_
#define _USRPROG_PROCESS_H_

#include "thread.h"

// 用户栈虚拟地址：用户空间顶端地址为 0xC0000000，最顶端的一个页用作栈
#define USER_STACK_VADDR (0xC0000000 - 0x1000)
// 用户程序起始地址，参考linux
#define USER_VADDR_START (0x8048000)
// 用户进程默认优先级
#define USER_DEFAULT_PRIORITY (31)

/* 激活进程所需环境 */
void process_active(struct PCB_INFO* p_pcb);
/* 创建进程 */
void process_execute(void* filename, char* name);

#endif
