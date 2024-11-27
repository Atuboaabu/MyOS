[bits 32]
extern g_IDT_handle

section .data
global interrupt_hld_table
interrupt_hld_table:

%macro CREATE_INTERUPT_HLD 2
section .text
interrupt_%1_hld:
    %2
    push ds
    push es
    push fs
    push gs
    pushad

    mov al, 0x20
    out 0xA0, al
    out 0x20, al

    push %1
    call [g_IDT_handle + %1 * 4]
    jmp interrupt_exit

section .data
    dd interrupt_%1_hld
%endmacro

section .text
global interrupt_exit
    add esp, 4
    popad
    pop gs
    pop fs
    pop es
    pop ds
    add esp, 4
    iretd

CREATE_INTERUPT_HLD 0x00, push 0
CREATE_INTERUPT_HLD 0x01, push 0
CREATE_INTERUPT_HLD 0x02, push 0
CREATE_INTERUPT_HLD 0x03, push 0
CREATE_INTERUPT_HLD 0x04, push 0
CREATE_INTERUPT_HLD 0x05, push 0
CREATE_INTERUPT_HLD 0x06, push 0
CREATE_INTERUPT_HLD 0x07, push 0
CREATE_INTERUPT_HLD 0x08, nop
CREATE_INTERUPT_HLD 0x09, push 0
CREATE_INTERUPT_HLD 0x0A, nop
CREATE_INTERUPT_HLD 0x0B, nop
CREATE_INTERUPT_HLD 0x0C, push 0
CREATE_INTERUPT_HLD 0x0D, nop
CREATE_INTERUPT_HLD 0x0E, nop
CREATE_INTERUPT_HLD 0x0F, push 0
CREATE_INTERUPT_HLD 0x10, push 0
CREATE_INTERUPT_HLD 0x11, nop
CREATE_INTERUPT_HLD 0x12, push 0
CREATE_INTERUPT_HLD 0x13, push 0
CREATE_INTERUPT_HLD 0x14, push 0
CREATE_INTERUPT_HLD 0x15, push 0
CREATE_INTERUPT_HLD 0x16, push 0
CREATE_INTERUPT_HLD 0x17, push 0
CREATE_INTERUPT_HLD 0x18, nop
CREATE_INTERUPT_HLD 0x19, push 0
CREATE_INTERUPT_HLD 0x1A, nop
CREATE_INTERUPT_HLD 0x1B, nop
CREATE_INTERUPT_HLD 0x1C, push 0
CREATE_INTERUPT_HLD 0x1D, nop
CREATE_INTERUPT_HLD 0x1E, nop
CREATE_INTERUPT_HLD 0x1F, push 0
CREATE_INTERUPT_HLD 0x20, push 0