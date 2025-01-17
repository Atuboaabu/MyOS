#include "print.h"
#include "interrupt.h"
#include "timer.h"
#include "debug.h"
#include "memory.h"
#include "thread.h"
#include "console.h"
#include "keyboard.h"

extern struct ioqueue g_keyboardIOQueue;
void thread_A(void *arg);

int main(void) {
    put_str("\nkernel start!\n");
    idt_init();
    timer_init();
    memory_init();
    thread_init();
    console_init();
    keyboard_init();

    thread_create("ThreadA", 20, thread_A, NULL);
    interrupt_enable();
    while(1) {
        // console_put_str("Main  ");
    }
    return 0;
}

void thread_A(void *arg) {
    while(1) {
        char c = ioqueue_getchar(&g_keyboardIOQueue);
        console_put_char(c);
    }
}