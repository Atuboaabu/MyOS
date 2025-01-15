#include "interrupt.h"
#include "boot.h"
#include "io.h"
#include "print.h"

/* 中断处理函数入口声明，定义在kernel.asm中 */
extern void* interrupt_hld_table[IDT_SIZE];
/* IDT中断处理函数数组 */
void* g_IDT_handle[IDT_SIZE];
/* IDT中断描述符表数组 */
struct gate_desc g_IDT[IDT_SIZE];

/* 创建中断描述符 */
void make_idt_desc(struct gate_desc* pdesc, uint8_t attribute, void* interrupt_handle)
{
    pdesc->text_offset_low = ((uint32_t)interrupt_handle) & 0x0000FFFF;
    pdesc->text_offset_high = (((uint32_t)interrupt_handle) & 0xFFFF0000) >> 16;
    pdesc->selector = SL_CODE;
    pdesc->dcount = 0;
    pdesc->attribute = attribute;
}

/* 中断描述符初始化 */
void idt_desc_init()
{
    put_str("idt desc init start!\n");
    for (int i = 0; i < IDT_SIZE; i++) {
        make_idt_desc(&g_IDT[i], IDT_DESC_ATTR_DPL0, interrupt_hld_table[i]);
    }
    put_str("idt desc init done!\n");
}

/* 中断处理函数 */
void interrupt_handle(uint8_t intr_vec_num)
{
    put_str("interrupt occur: vec_num = ");
    put_int(intr_vec_num);
    put_char('\n');

    if (intr_vec_num == 14) {	  // 若为Pagefault,将缺失的地址打印出来并悬停
      int page_fault_vaddr = 0; 
      asm ("movl %%cr2, %0" : "=r" (page_fault_vaddr));	  // cr2是存放造成page_fault的地址
      put_str("\npage fault addr is ");
      put_int(page_fault_vaddr); 
   }
   while(1);
}

/* 中断处理函数表初始化注册 */
void exception_init()
{
    put_str("exception init start!\n");
    for (int i = 0; i < IDT_SIZE; i++) {
        g_IDT_handle[i] = interrupt_handle;
    }
    put_str("exception init done!\n");
}

/* 注册中断处理函数 */
void register_interrupt_handle(uint8_t intr_vec_num, void (*handle)(void))
{
    if (intr_vec_num >= IDT_SIZE) {
        return;
    }
    put_str("register_interrupt_handle: vec num = ");
    put_int(intr_vec_num);
    put_char('\n');
    g_IDT_handle[intr_vec_num] = handle;
}

/* 初始化可编程中断控制器 8259A */
void pic_init()
{
    put_str("pic init start!\n");
    /* 主片初始化 */
    outb(PIC_M_CTRL, 0x11);
    outb(PIC_M_DATA, 0x20);
    outb(PIC_M_DATA, 0x04);
    outb(PIC_M_DATA, 0x01);
    /* 从片初始化 */
    outb(PIC_S_CTRL, 0x11);
    outb(PIC_S_DATA, 0x28);
    outb(PIC_S_DATA, 0x02);
    outb(PIC_S_DATA, 0x01);
    /* 打开主片IR0 */
    outb(PIC_M_DATA, 0xFC);
    outb(PIC_S_DATA, 0xFF);

    put_str("pic init done!\n");
}

/* 开启中断 */
void interrupt_enable()
{
    asm volatile("sti");  // Set Interrupt Flag
}

/* 关闭中断 */
void interrupt_disable()
{
    asm volatile("cli");  // Clear Interrupt Flag
}

/* 设置中断的状态中断 */
void set_interrupt_status(enum interrupt_status status)
{
    if (status == INTERRUPT_ENABLE) {
        interrupt_enable();
    } else {
        interrupt_disable();
    }
}

/* 获取中断的状态中断 */
enum interrupt_status get_interrupt_status()
{
    uint32_t eflags = 0; 
    asm volatile("pushfl; popl %0" : "=g" (eflags));
    return (EFLAGS_IF & eflags) ? INTERRUPT_ENABLE : INTERRUPT_DISABLE;
}

/* 中断部分初始化函数 */
void idt_init()
{
    put_str("idt init start!\n");
    idt_desc_init();
    exception_init();
    pic_init();

    /* 加载 IDT */
    uint64_t idt_oper = ((sizeof(g_IDT) - 1) | ((uint64_t)(uint32_t)g_IDT << 16));
    asm volatile("lidt %0" : : "m" (idt_oper));

    put_str("idt init done!\n");
}