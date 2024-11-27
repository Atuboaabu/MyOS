#include "interrupt.h"

void* g_IDT_handle[IDT_SIZE];

void interrupt_handle(uint8_t intr_vec_num)
{
    put_str("interrupt occur: vec_num = ");
    put_int(intr_vec_num);
    put_char('\n');
}