[bits 32]
section .text
global switch_to
switch_to:
    ;栈中第一个参数为函数返回值的地址：retaddr；4字节   
    push esi
    push edi
    push ebx
    push ebp

    mov eax, [esp + 20] ; 得到栈中的参数cur_pcb
    mov [eax], esp      ; 保存栈顶指针esp -> cur_pcb.self_kstack

    ; 以上是备份当前线程的环境，下面是恢复下一个线程的环境
    mov eax, [esp + 24]  ; 得到栈中的参数next_pcb
    mov esp, [eax]       ; 恢复下一个线程栈指针: esp <- next_pcb.self_kstack,

    pop ebp
    pop ebx
    pop edi
    pop esi
    ret
