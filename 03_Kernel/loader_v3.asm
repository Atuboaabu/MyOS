;------------------------------------------------------------
%include "boot.inc"
SECTION LOADER vstart=LOADER_BASE_ADDR  
    LOADER_STACK_TOP equ LOADER_BASE_ADDR
    jmp near loader_start  ; 跳转执行加载

;----- GDT定义 START -----
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
;----- GDT定义 END -----

;----- 字符串定义 START -----
    memgetmsg db "Memory get:"
    memgetover  db "Memory get over!"
    loadermsg db "Loader start in real mode!"

    mem_map_entry times 24 db 0
;----- 字符串定义 END -----

;----- 变量定义 START -----
    print_line dw 2 ;当前的打印行，用于设置打印位置
;----- 变量定义 END -----

loader_start:
    mov	sp, LOADER_BASE_ADDR
    ;-------- 获取物理内存 -------
    ;输入参数：
    ;EAX = 0xE820：表示请求内存区域信息。
    ;EBX = 0：为零，表示从地址 0 开始查询内存区域。
    ;ECX = 24：表示请求返回的信息块的大小（字节数），通常为 24 字节（每个内存块）。
    ;EDX = 0x534D4150（'SMAP'）：用于确认调用是否有效，通常使用此值表示支持该调用。
    ;返回值：
    ;ESI：返回内存块的物理起始地址。
    ;ECX：返回内存块的大小（单位：字节）。
    ;EBX：返回内存块类型。
    ;EAX：返回一个状态码，通常是 0x534D4150（'SMAP'）来标识成功。

    ;----- 打印 memgetmsg -----
    mov	 bp, memgetmsg   ; ES:BP = 字符串地址
    mov	 cx, 11			 ; CX = 字符串长度
    mov	 ax, 0x1301		 ; AH = 13,  AL = 01h
    mov	 bx, 0x001F		 ; 页号为0(BH = 0) 蓝底粉红字(BL = 1Fh)
    mov	 dh, [print_line]
    mov	 dl, 0
    inc word [print_line]
    int	 0x10            ; 10h 号中断

    ;----- 获取物理内存 -----
    xor ebx, ebx
get_memory:
    mov edx, 0x534D4150
    mov eax, 0x0000E820
    mov ecx, 24
    mov di, mem_map_entry ;查询到的内存区域
    int 0x15
    jc memory_get_done

    cmp eax, 0x534D4150     ; 检查返回值是否为"SMAP"
    jne memory_get_done     ; 如果没有获取到有效信息，则退出

    test ebx, ebx
    jz memory_get_done

    mov si, mem_map_entry
    mov cx, 24

    ; ----- 设置光标位置 (行：print_line, 列 0) -----
    mov ah, 2               ; 功能号：设置光标位置
    mov bh, 0               ; 页号
    mov dh, [print_line]    ; 行号
    mov dl, 0               ; 列号
    inc word [print_line]
    int 0x10

    ;----- 打印获取的存信息 -----
print_mem:
    ;高4位打印
    mov al, [si]
    shr al, 4
    and al, 0x0F
    cmp al, 9
    jbe digit_low
digit_high:
    add al, 'A' - 10
    jmp digital_result
digit_low:
    add al, '0'
digital_result:
    mov ah, 0x0E
    int 0x10

    ;低4位打印
    mov al, [si]
    and al, 0x0F
    cmp al, 9
    jbe digit_low_2
digit_high_2:
    add al, 'A' - 10
    jmp digital_result_2
digit_low_2:
    add al, '0'
digital_result_2:
    mov ah, 0x0E
    int 0x10

    inc si
    loop print_mem ;循环打印24位内存信息

    jmp get_memory ;获取下一个内存信息

memory_get_done:
    ;----- 打印 memgetover -----
    mov	 bp, memgetover   ; ES:BP = 字符串地址
    mov	 cx, 16			 ; CX = 字符串长度
    mov	 ax, 0x1301		 ; AH = 13,  AL = 01h
    mov	 bx, 0x001F		 ; 页号为0(BH = 0) 蓝底粉红字(BL = 1Fh)
    mov	 dh, [print_line]
    mov	 dl, 0
    inc word [print_line]
    int	 0x10            ; 10h 号中断

    ;-------- 打印 loadermsg --------
    mov	 bp, loadermsg   ; ES:BP = 字符串地址
    mov	 cx, 26			 ; CX = 字符串长度
    mov	 ax, 0x1301		 ; AH = 13,  AL = 01h
    mov	 bx, 0x001F		 ; 页号为0(BH = 0) 蓝底粉红字(BL = 1Fh)
    mov	 dh, [print_line]
    mov	 dl, 0
    inc word [print_line]
    int	 0x10            ; 10h 号中断

    ;--------2、进入保护模式--------
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
    mov gs, ax

    ;创建页表
    call setup_page

    ;要将描述符表地址及偏移量写入内存gdt_ptr,一会用新地址重新加载
    sgdt [gdt_ptr]

    ;将gdt描述符中视频段描述符中的段基址 + 0xc0000000
    mov ebx, [gdt_ptr + 2] 
    ;视频段是第3个段描述符,每个描述符是8字节,故0x18。
    ;段描述符的高4字节的最高位是段基址的31~24位
    or dword [ebx + 0x18 + 4], 0xc0000000
                            
    ;将gdt的基址加上0xc0000000使其成为内核所在的高地址, 把 GDT 也移到内核空间中
    add dword [gdt_ptr + 2], 0xc0000000

    ;将栈指针同样映射到内核地址
    add esp, 0xc0000000

    ;把页目录地址赋给cr3
    mov eax, PAGE_BASE_ADDR
    mov cr3, eax

    ;打开cr0的pg位(第31位)
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    ;在开启分页后,用gdt新的地址重新加载
    lgdt [gdt_ptr]             ; 重新加载

    mov byte [gs:1600], "V"
    mov byte [gs:1601], 0x07
    jmp $

;-------- 页表创建 --------
;-------- 1、清空页表目录对应内存 --------
setup_page:
    mov ecx, 4096
    mov esi, 0
.clear_page_dir:
    mov byte[PAGE_BASE_ADDR + esi], 0
    inc esi
    loop .clear_page_dir

;-------- 2、创建第 1 个PDE --------
.create_pde:
    mov eax, PAGE_BASE_ADDR
    add eax, 0x1000
    mov ebx, eax

    or eax, PG_US_U | PG_RW_W | PG_P
    ;将页表地址和属性写入第一个页目录项（用于映射 0x00000000~0x003FFFFF）。
    mov [PAGE_BASE_ADDR + 0], eax
    ;设置第768个目录项，使 0xC0000000~0xC03FFFFF 映射到相同的物理地址。这用于内核地址映射。
    mov [PAGE_BASE_ADDR + 0xC00], eax

;-------- 3、最末尾的 1 个PDE，使其指向PDE的起始位置 --------
    sub eax, 0x1000
    mov [PAGE_BASE_ADDR + 4092], eax

;-------- 4、创建第 1 个PDE 对应的页表项PTE --------
    mov ecx, 256 ;每个页表对应大小为4KB，低端1MB地址对应 1MB / 4KB = 256个PTE
    mov esi, 0
    mov edx, PG_US_U | PG_RW_W | PG_P
.create_pte: ;对应地址0~0x10000
    mov [ebx + esi * 4], edx
    add edx, 0x1000
    inc esi
    loop .create_pte

;-------- 5、创建内核的其他 PDE --------
    mov eax, PAGE_BASE_ADDR
    add eax, 0x2000
    or eax, PG_US_U | PG_RW_W | PG_P
    mov ebx, PAGE_BASE_ADDR
    mov ecx, 254 ; 范围为第769~1022的所有目录项数量
    mov esi, 769
.create_kernel_pde:
    mov [ebx + esi * 4], eax
    inc esi
    add eax, 0x1000
    loop .create_kernel_pde
    ret