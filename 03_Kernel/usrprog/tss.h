#ifndef _USRPROG_TSS_H_
#define _USRPROG_TSS_H_

#include "stdint.h"
#include "memory.h"

struct tss {
    uint32_t prev_tss;  // 上一个任务的TSS指针
    uint32_t* esp0;
    uint32_t ss0;
    uint32_t* esp1;
    uint32_t ss1;
    uint32_t* esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t (*eip) (void);
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint32_t trap;
    uint32_t io_map_base;
};

void tss_init();
void tss_update_esp0(struct PCB_INFO* p_pcb);

#endif