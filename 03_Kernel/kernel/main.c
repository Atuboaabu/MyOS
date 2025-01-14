#include "print.h"
#include "interrupt.h"
#include "timer.h"
#include "debug.h"
#include "memory.h"
#include "thread.h"
#include "console.h"

void thread_A(void *arg);
void thread_B(void *arg);

int main(void) {
    put_str("\nkernel start!\n");
    idt_init();
    timer_init();
    memory_init();
    thread_init();
    console_init();

    // void* vaddr = (uint32_t)get_kernel_pages(1);
    // if (vaddr != NULL) {
    //     put_str("\nget kernel pages vaddr = ");
    //     put_int((uint32_t)vaddr);
    //     put_str("\n");
    // }
    thread_create("ThreadA", 40, thread_A, NULL);
    thread_create("ThreadB", 40, thread_B, NULL);
    interrupt_enable();
    // for (int i = 10000000; i > 0; i--) {

    // }
    // ASSERT(1 == 0);
    while(1) {
        console_put_str("Main  ");
    }
    return 0;
}

void thread_A(void *arg) {
    while(1) {
        console_put_str("Thread A  ");
    }
}

void thread_B(void *arg) {
    while(1) {
        console_put_str("Thread B  ");
    }
}