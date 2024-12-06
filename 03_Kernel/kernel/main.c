#include "print.h"
#include "interrupt.h"
#include "timer.h"
#include "debug.h"
#include "memory.h"

int main(void) {
    put_str("\nkernel start!\n");
    idt_init();
    timer_init();
    memory_init();
    interrupt_enable();
    // for (int i = 10000000; i > 0; i--) {

    // }
    ASSERT(1 == 0);
    while(1) {};
    return 0;
}