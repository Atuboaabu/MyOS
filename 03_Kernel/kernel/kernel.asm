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
interrupt_exit:
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
CREATE_INTERUPT_HLD 0x20, push 0 ;时钟中断
CREATE_INTERUPT_HLD 0x21, push 0 ;键盘中断
CREATE_INTERUPT_HLD 0x22, push 0 ;级联使用
CREATE_INTERUPT_HLD 0x23, push 0
CREATE_INTERUPT_HLD 0x24, push 0
CREATE_INTERUPT_HLD 0x25, push 0
CREATE_INTERUPT_HLD 0x26, push 0
CREATE_INTERUPT_HLD 0x27, push 0
CREATE_INTERUPT_HLD 0x28, push 0
CREATE_INTERUPT_HLD 0x29, push 0
CREATE_INTERUPT_HLD 0x2A, push 0
CREATE_INTERUPT_HLD 0x2B, push 0
CREATE_INTERUPT_HLD 0x2C, push 0
CREATE_INTERUPT_HLD 0x2D, push 0
CREATE_INTERUPT_HLD 0x2E, push 0 ;硬盘
CREATE_INTERUPT_HLD 0x2F, push 0

;;;;; 0x80 中断处理流程 ;;;;;
[bits 32]
extern g_Syscall_Table
section .text
global syscall_handler
syscall_handler:
    push 0           ; 压入0, 保持栈中格式统一
    push ds
    push es
    push fs
    push gs
    pushad
    push 0x80        ; 压入 0x80 中断号，保持统一的栈格式

    ; 为系统调用子功能传入参数
    push edx         ; 系统调用中第3个参数
    push ecx         ; 系统调用中第2个参数
    push ebx         ; 系统调用中第1个参数

    ; 调用子功能处理函数
    call [g_Syscall_Table + eax * 4]
    add esp, 12      ; 跨过上面的三个参数

    ; 将call调用后的返回值存入待当前内核栈中eax的位置
    mov [esp + 8 * 4], eax
    jmp interrupt_exit    ; intr_exit返回, 恢复上下文