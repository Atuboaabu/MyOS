;------------------------------------------------------------
%include "boot.inc"
SECTION LOADER vstart=LOADER_BASE_ADDR  
    LOADER_STACK_TOP equ LOADER_BASE_ADDR
    jmp near loader_start  ; 跳转执行加载

GDT_START:
    ; GDT 第一个描述符为NULL描述符
    dd 0x00000000
    dd 0x00000000
GDT_CODE:
    dd 0x0000FFFF
    dd DGT_CODE_H32
GDT_STACK_DATA:
    dd 0x0000FFFF
    dd DGT_DATA_H32
GDT_VIDEO_DATA:
    dd 0x80000007 ;内存区间：0xB8000~0xBFFFF, limit=(0xbffff-0xb8000)/4k=0x7
    dd DGT_VIDEO_H32

    GDT_SIZE  equ $ - GDT_START
    GDT_LIMIT equ GDT_SIZE - 1

    times 50 dq 0 ;定义一组 64 位（8 字节）数据项
    SL_CODE  equ (0x0001 << 3) + SL_TI_GDT + SL_RPL0 ;代码段选择子
    SL_DATA  equ (0x0002 << 3) + SL_TI_GDT + SL_RPL0 ;数据段选择子
    SL_VIDEO equ (0x0003 << 3) + SL_TI_GDT + SL_RPL0 ;显存段选择子

    gdt_ptr dw GDT_LIMIT
            dd GDT_START
    
    loadermsg db 'Loader start in real mode!'

loader_start:
    ;--------1、打印字符串--------
    mov	 sp, LOADER_BASE_ADDR
    mov	 bp, loadermsg   ; ES:BP = 字符串地址
    mov	 cx, 26			 ; CX = 字符串长度
    mov	 ax, 0x1301		 ; AH = 13,  AL = 01h
    mov	 bx, 0x001F		 ; 页号为0(BH = 0) 蓝底粉红字(BL = 1Fh)
    mov	 dx, 0x1800		 ;
    int	 0x10            ; 10h 号中断

    ;--------2、进入实模式--------
    ;--------2.1、打开A20--------
    in al, 0x92
    or al, 0x02
    out 0x92, al

    ;--------2.2、加载gdt--------
    lgdt [gdt_ptr]

    ;--------2.3、cr0第0位置1，开启保护模式--------
    mov eax, cr0
    or eax, 0x00000001
    mov cr0, eax

    jmp dword SL_CODE:p_mode_start ; 刷新流水线，避免分支预测的影响，这种cpu优化策略，最怕jmp跳转，
					                ; 这将导致之前做的预测失效，从而起到了刷新的作用。
    
[bits 32]
p_mode_start:
    mov ax, SL_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, LOADER_STACK_TOP
    mov ax, SL_VIDEO

    mov byte [gs:0xA0], 'L'
    mov byte [gs:0xA1], 0x07 ;0x07 = 0000 0111, 黑色背景，白色字体
    mov byte [gs:0xA2], 'O'
    mov byte [gs:0xA3], 0x07
    mov byte [gs:0xA4], 'A'
    mov byte [gs:0xA5], 0x07
    mov byte [gs:0xA6], 'D'
    mov byte [gs:0xA7], 0x07
    mov byte [gs:0xA8], 'E'
    mov byte [gs:0xA9], 0x07
    mov byte [gs:0xAA], 'R'
    mov byte [gs:0xAB], 0x07
    mov byte [gs:0xAC], ' '
    mov byte [gs:0xAD], 0x07
    mov byte [gs:0xAE], 'S'
    mov byte [gs:0xAF], 0x07
    mov byte [gs:0xB0], 't'
    mov byte [gs:0xB1], 0x07
    mov byte [gs:0xB2], 'a'
    mov byte [gs:0xB3], 0x07
    mov byte [gs:0xB4], 'r'
    mov byte [gs:0xB5], 0x07
    mov byte [gs:0xB6], 't'
    mov byte [gs:0xB7], 0x07
    mov byte [gs:0xB8], '!'
    mov byte [gs:0xB9], 0x07

    jmp $