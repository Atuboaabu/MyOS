#include "print.h"
#include "interrupt.h"
#include "timer.h"

int main(void) {
    put_str("\nkernel start!\n");
    idt_init();
    timer_init();
    interrupt_enable();
    while(1) {};
    return 0;
}