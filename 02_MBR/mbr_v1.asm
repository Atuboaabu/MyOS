;------------------------------------------------------------
SECTION MBR vstart=0x7c00          
   mov ax,cs      
   mov ds,ax
   mov es,ax
   mov ss,ax
   mov fs,ax
   mov sp,0x7c00 ;栈指针
   mov ax, 0xb800
   mov gs, ax
 
; 清屏 利用0x06号功能，上卷全部行，则可清屏。
; -----------------------------------------------------------
;INT 0x10   功能号:0x06       功能描述:上卷窗口
;------------------------------------------------------
;输入：
;AH 功能号= 0x06
;AL = 上卷的行数(如果为0,表示全部)
;BH = 上卷行属性
;(CL,CH) = 窗口左上角的(X,Y)位置
;(DL,DH) = 窗口右下角的(X,Y)位置
;无返回值：
   mov     ax, 0x0600
   mov     bx, 0x0700
   mov     cx, 0           ; 左上角: (0, 0)
   mov     dx, 0x184f       ; 右下角: (80,25),
               ; VGA文本模式中,一行只能容纳80个字符,共25行。
               ; 下标从0开始,所以0x18=24,0x4f=79
   int     0x10            ; int 0x10
 
;;;;;;;;;    下面这三行代码是获取光标位置    ;;;;;;;;;
;.get_cursor获取当前光标位置,在光标位置处打印字符.
   mov ah, 3        ; 输入: 3号子功能是获取光标位置,需要存入ah寄存器
   mov bh, 0        ; bh寄存器存储的是待获取光标的页号
 
   int 0x10        ; 输出: ch=光标开始行,cl=光标结束行
            ; dh=光标所在行号,dl=光标所在列号
 
;;;;;;;;;    获取光标位置结束    ;;;;;;;;;;;;;;;;
 
;;;;;;;;;     打印字符串    ;;;;;;;;;;;
   mov byte [gs:0x00], 'M'
   mov byte [gs:0x01], 0xa4 ;0xa4 = 0x1010 0100, 绿色背景闪烁，文字颜色为红色
   mov byte [gs:0x02], 'B'
   mov byte [gs:0x03], 0xa4
   mov byte [gs:0x04], 'R'
   mov byte [gs:0x05], 0xa4
   mov byte [gs:0x06], ' '
   mov byte [gs:0x07], 0xa4
   mov byte [gs:0x08], 'S'
   mov byte [gs:0x09], 0xa4
   mov byte [gs:0x0A], 't'
   mov byte [gs:0x0B], 0xa4
   mov byte [gs:0x0C], 'a'
   mov byte [gs:0x0D], 0xa4
   mov byte [gs:0x0E], 'r'
   mov byte [gs:0x0F], 0xa4
   mov byte [gs:0x10], 't'
   mov byte [gs:0x11], 0xa4
   mov byte [gs:0x12], ' '
   mov byte [gs:0x13], 0xa4
   mov byte [gs:0x14], 'N'
   mov byte [gs:0x15], 0xa4
   mov byte [gs:0x16], 'o'
   mov byte [gs:0x17], 0xa4
   mov byte [gs:0x18], 'w'
   mov byte [gs:0x19], 0xa4
   mov byte [gs:0x1A], '!'
   mov byte [gs:0x1B], 0xa4
;;;;;;;;;      打字字符串结束     ;;;;;;;;;;;;;;;
 
   jmp $        ; 使程序悬停在此
 
   message db "MBR Start Now!"
   times 510-($-$$) db 0
   db 0x55,0xaa