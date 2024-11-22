;------------------------------------------------------------
%include "boot.inc"
SECTION MBR vstart=0x7c00          
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov sp, 0x7c00
    mov ax, 0xb800
    mov gs, ax

; -----------------------------------------------------
; INT 0x10   功能号:0x06       功能描述:上卷窗口
;------------------------------------------------------
    mov ax, 0x0600
    mov bx, 0x0700
    mov cx, 0
    mov dx, 0x184F ; VGA文本模式中,一行只能容纳80个字符,共25行。
                   ; 下标从0开始,所以0x18=24,0x4f=79
    int 0x10

; -----------------------------------------------------
; 获取光标位置
; -----------------------------------------------------
    mov ah, 3      ; 输入: 3号子功能是获取光标位置,需要存入ah寄存器
    mov bh, 0      ; bh寄存器存储的是待获取光标的页号
    int 0x10       ; 输出: ch=光标开始行,cl=光标结束行

; -----------------------------------------------------
; 打印字符串
; -----------------------------------------------------
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

; -----------------------------------------------------------
; 读取扇区：参数初始化&调用
; -----------------------------------------------------------
    mov eax, LOADER_START_SECTOR ;起始扇区1ba的地址
    mov bx, LOADER_BASE_ADDR     ;写入的地址
    mov cx, 4                    ;待读入的扇区数
    call rd_disk_m_16

   jmp LOADER_BASE_ADDR

; -----------------------------------------------------------
;功能：读取硬盘n个扇区
;------------------------------------------------------------
rd_disk_m_16:
    ;输入: eax=LBA扇区号；ebx=扇区起始地址；ecx=读入的扇区数
    mov esi, eax ;备份eax
    mov di, cx   ;备份cx

    ;读写硬盘
    ;第1步：设置需要读取的扇区数0x1f2
    mov dx, 0x1F2 ;sector count寄存器
    mov al, cl    ;扇区数
    out dx, al
    mov eax, esi  ;恢复eax值

    ;第2步：将LBA地址存入0x1f3-0x1f6
    mov dx, 0x1F3 ;LBA low寄存器
    out dx, al

    mov dx, 0x1F4 ;LBA mid寄存器
    mov cl, 8
    shr eax, cl   ;eax右移8位，取mid地址
    out dx, al

    mov dx, 0x1F5 ;LBA high寄存器
    shr eax, cl   ;eax再右移8位，取high地址
    out dx, al

    mov dx, 0x1F6 ;device寄存器低4位，对应地址的最高4位23~27位
    shr eax, cl   ;eax再右移8位，取最高位地址
    and al, 0x0F  ;只有4位，取个与
    or al, 0xE0   ;设置devie寄存器其余bit位，1110表示LBA模式，从盘
    out dx, al

    ; 第3步：向command寄存器寄存器写入命令；0X1F7
    mov dx, 0x1F7
    mov al, 0x20  ;读操作
    out dx, al

    ; 第4步：检查硬盘状态
.not_ready:
    nop
    in al, dx
    and al, 0x88   ;判断第3位(data request)、第7位(BSY位)
    cmp al, 0x08   ;数据是否就绪
    jnz .not_ready ;未就绪就等待

    ;第5步：读取数据，0x1F0
    mov ax, di
    mov dx, 256
    mul dx
    mov cx, ax    ;1个扇区有51个字节，每次读入一个字节，共需要读512 * di / 2次

    mov dx, 0x1F0
.go_on_read:
    in ax, dx
    mov [bx], ax
    add bx, 2
    loop .go_on_read
    ret

    times 510-($-$$) db 0
    db 0x55,0xaa