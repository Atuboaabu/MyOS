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

    thread_create("ThreadA", 2, thread_A, NULL);
    thread_create("ThreadB", 2, thread_B, NULL);
    interrupt_enable();
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