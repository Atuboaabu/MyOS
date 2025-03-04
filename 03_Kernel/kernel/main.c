#include "print.h"
#include "interrupt.h"
#include "timer.h"
#include "debug.h"
#include "memory.h"
#include "thread.h"
#include "console.h"
#include "keyboard.h"
#include "tss.h"
#include "process.h"
#include "syscall.h"
#include "stdio.h"
#include "ide.h"
#include "fs.h"
#include "shell.h"
#include "fork.h"

extern struct ioqueue g_keyboardIOQueue;
void thread_A(void *arg);
void process_A();
int a = 0;

int main(void) {
    put_str("\nkernel start!\n");
    idt_init();
    timer_init();
    memory_init();
    thread_init();
    console_init();
    keyboard_init();
    tss_init();
    syscall_init();
    ide_init();
    file_system_init();

    // thread_create("ThreadA", 20, thread_A, NULL);
    // thread_create("process_A", 20, process_A, NULL);
    process_execute(process_A, "process_A");
    // process_execute(shell_process, "shell");
    interrupt_enable();
    while(1) {
        // console_put_str("Main  ");
    }
    return 0;
}

void thread_A(void *arg) {
    // sys_write("thread_A \n");
    while(1) { 
    }
}

void process_A(void *arg) {
    printf("process_A pid = %d \n", getpid());
    void* addr = malloc(17);
    free(addr);
    void* addr1 = malloc(19);
    printf("addr = 0x%x  \n", (uint32_t)addr);
    printf(" addr1 = 0x%x \n", (uint32_t)addr1);
    int32_t pid = fork();
    if (pid == 0) {
        printf("I'm child process, pid = %d \n", getpid());
    } else if (pid > 0) {
        printf("I'm parend process, my child pid = %d \n", pid);
    } else {
        printf("fork error!\n");
    }
    while(1) {
        // a = getpid();
    }
}