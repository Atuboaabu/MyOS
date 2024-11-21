;------------------ loader -----------------
LOADER_BASE_ADDR    equ 0x900
LOADER_START_SECTOR equ 0x2

;------------------ kernel -----------------
KERNEL_BASE_ADDR    equ 0x70000
KERNEL_START_SECTOR equ 0x9
KERNEL_ENTRY_ADDR   equ 0xC0001500

;----------------------- GDT --------------------------
; GDT各个参数定义，将GDT人为分为高32位、低32位看
; 此处的定义都是在高32位的值，SD是Segment Description缩写
;------------------------------------------------------
SD_G_4K      equ 0x00800000
SD_DB_32     equ 0x00400000
SD_L_64      equ 0x00200000
SD_AVL_1     equ 0x00100000
SD_P_1       equ 0x00008000
SD_DPL_0     equ 0x00000000
SD_DPL_1     equ 0x00002000
SD_DPL_2     equ 0x00004000
SD_DPL_3     equ 0x00006000
SD_S_1       equ 0x00001000
SD_TYPE_CODE equ 0x00000800 ;x=1,c=0,r=0,a=0 代码段是可执行的,非依从的,不可读的,已访问位a清0
SD_TYPE_DATA equ 0x00000200 ;x=0,e=0,w=1,a=0 数据段是不可执行的,向上扩展的,可写的,已访问位a清0

SD_CODE_LH   equ 0x000F0000
SD_DATA_LH   equ 0x000F0000
SD_DATA_VD   equ 0x00000000

; GDT 代码段描述符：BH=0, G=1, D/B=1, L=AVL=0, LH=0xF, P=1, DPL=0, S=1, Type=0x8, BM=0
DGT_CODE_H32  equ 0x00000000 + SD_G_4K + SD_DB_32 + SD_CODE_LH + SD_P_1 + SD_DPL_0 + SD_S_1 + SD_TYPE_CODE

; GDT Data段描述符：BH=0, G=1, D/B=1, L=AVL=0, LH=0xF, P=1, DPL=0, S=1, Type=0x8, BM=0
DGT_DATA_H32  equ 0x00000000 + SD_G_4K + SD_DB_32 + SD_DATA_LH + SD_P_1 + SD_DPL_0 + SD_S_1 + SD_TYPE_DATA

; GDT Video段描述符：BH=0, G=1, D/B=1, L=AVL=0, LH=0xF, P=1, DPL=0, S=1, Type=0x8, BM=0
DGT_VIDEO_H32 equ 0x00000000 + SD_G_4K + SD_DB_32 + SD_DATA_VD + SD_P_1 + SD_DPL_0 + SD_S_1 + SD_TYPE_DATA + 0x0B

;----------------------- Selector --------------------------
; 选择子各个参数定义
;-----------------------------------------------------------
SL_RPL0   equ 00b
SL_RPL1   equ 01b
SL_RPL2   equ 10b
SL_RPL3   equ 11b
SL_TI_GDT equ 000b
SL_TI_LDT equ 100b

;----------------------- Page Table ------------------------
; 页表相关属性定义
;-----------------------------------------------------------
PAGE_BASE_ADDR    equ 0x100000

PG_P              equ   1b
PG_RW_R           equ  00b 
PG_RW_W           equ  10b 
PG_US_S           equ  000b 
PG_US_U           equ  100b 
