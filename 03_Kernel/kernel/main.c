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
    void* vaddr = (uint32_t)get_kernel_pages(1);
    if (vaddr != NULL) {
        put_str("\nget kernel pages vaddr = ");
        put_int((uint32_t)vaddr);
        put_str("\n");
    }
    interrupt_enable();
    // for (int i = 10000000; i > 0; i--) {

    // }
    ASSERT(1 == 0);
    while(1) {};
    return 0;
}