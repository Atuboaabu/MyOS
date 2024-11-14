;------------------------------------------------------------
%include "boot.inc"
SECTION LOADER vstart=LOADER_BASE_ADDR  
    mov byte [gs:0x00], 'L'
    mov byte [gs:0x01], 0xa4 ;0xa4 = 0x1010 0100, 绿色背景闪烁，文字颜色为红色
    mov byte [gs:0x02], 'O'
    mov byte [gs:0x03], 0xa4
    mov byte [gs:0x04], 'A'
    mov byte [gs:0x05], 0xa4
    mov byte [gs:0x06], 'D'
    mov byte [gs:0x07], 0xa4
    mov byte [gs:0x08], 'E'
    mov byte [gs:0x09], 0xa4
    mov byte [gs:0x0A], 'R'
    mov byte [gs:0x0B], 0xa4
    mov byte [gs:0x0C], ' '
    mov byte [gs:0x0D], 0xa4
    mov byte [gs:0x0E], 'S'
    mov byte [gs:0x0F], 0xa4
    mov byte [gs:0x10], 't'
    mov byte [gs:0x11], 0xa4
    mov byte [gs:0x12], 'a'
    mov byte [gs:0x13], 0xa4
    mov byte [gs:0x14], 'r'
    mov byte [gs:0x15], 0xa4
    mov byte [gs:0x16], 't'
    mov byte [gs:0x17], 0xa4
    mov byte [gs:0x18], '!'
    mov byte [gs:0x19], 0xa4
    ;;;;;;;;;      打字字符串结束     ;;;;;;;;;;;;;;;
    ;jmp $