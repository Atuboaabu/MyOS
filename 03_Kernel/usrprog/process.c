#include "process.h"
#include "thread.h"
#include "string.h"
#include "boot.h"

void process_start(void* _filename) {
    void* func = _filename;
    struct PCB_INFO* cur_pcb = get_curthread_pcb();
    /* 获取中断栈指针 */
    cur_pcb->self_kstack += sizeof(struct THREAD_STACK);
    struct INTR_STACK* intr_stack = (struct INTR_STACK*)cur_pcb->self_kstack;
    memset(intr_stack, 0, sizeof(struct INTR_STACK));
    intr_stack->ds = SL_USRDATA;
    intr_stack->es = SL_USRDATA;
    intr_stack->fs = SL_USRDATA;
    intr_stack->eip = fucn;
    intr_stack->cs = SL_USRCODE;
    struct EFLAGS eflags = { 0 };
    eflags.IOPL = 0;
    eflags.MBS = 1;
    eflags.IF = 1;
    intr_stack->eflags = (uint32_t)eflags;



}