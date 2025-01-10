#ifndef _THREAD_THREAD_H_
#define _THREAD_THREAD_H_

#include "stdint.h"
#include "list.h"

/* 进程、线程状态枚举 */
enum TASK_STATUS {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DEAD
};

/* 中断栈内容 */
struct INTR_STACK {
    uint32_t vec_no;        // 中断号
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;    // 虽然pushad把esp也压入,但esp是不断变化的,所以会被popad忽略
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    /* 以下由cpu从低特权级进入高特权级时压入 */
    uint32_t err_code;    // err_code会被压入在eip之后
    void (*eip) (void);
    uint32_t cs;
    uint32_t eflags;
    void* esp;
    uint32_t ss;
};

/* 线程栈内容 */
struct THREAD_STACK {
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;
    /* 线程第一次执行时,eip指向待调用的函数kernel_thread
    ** 其它时候,eip是指向switch_to的返回地址*/
    void (*eip) (thread_func* func, void* func_arg);
    /*    以下仅供第一次被调度上cpu时使用     */
    /* 参数unused_ret只为占位置充数为返回地址 */
    void (*unused_retaddr);
    thread_func* function;   // 由Kernel_thread所调用的函数名
    void* func_arg;          // 由Kernel_thread所调用的函数所需的参数
};

/* 进程、线程的PCB：程序控制块 */
struct PCB_INFO {
    uint32_t* self_kstack;         // 各内核线程都用自己的内核栈
    enum TASK_STATUS status;
    char name[16];
    uint8_t priority;
    uint8_t ticks;                 // 每次在处理器上执行的时间嘀嗒数
    uint32_t elapsed_ticks;        // 此任务自上cpu运行后至今占用了多少cpu嘀嗒数

   struct list_elem ready_list_tag;   // 线程在一般的队列中的结点
   struct list_elem all_list_tag;  // 线程队列thread_all_list中的结点

   uint32_t* pgdir;               // 进程自己页表的虚拟地址
   uint32_t stack_magic;          // 用这串数字做栈的边界标记,用于检测栈的溢出
};

#endif