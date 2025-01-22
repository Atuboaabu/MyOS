#include "tss.h"
#include "print.h"
#include "string.h"
#include "boot.h"

static struct tss s_tss;
/* GDT描述符的起始地址 0x903 */
#define GDT_START_ADDR (0xC0000903)

void tss_init() {
    put_str("tss init start!\n");
    /* 初始化 tss */
    uint32_t tss_size = sizeof(s_tss);
    memset(&s_tss, 0, tss_size);
    s_tss.ss0 = SL_CODE;
    s_tss.io_map_base = tss_size;
    /* tss 描述符添加 */
    struct GDT_DESC tss_desc;
    uint32_t tss_desc_size = sizeof(tss_desc);
    memset(&tss_desc, 0, tss_desc_size);
    tss_desc.base_0_15 = ((uint32_t)(&s_tss) & 0x0000FFFF);
    tss_desc.base_16_23 = ((uint32_t)(&s_tss) & 0x00FF0000 >> 16);
    tss_desc.base_24_31 = ((uint32_t)(&s_tss) & 0xFF000000 >> 24);
    tss_desc.limit_0_15 = ((tss_size - 1) & 0x0000FFFF);
    tss_desc.limit_16_19 = ((tss_size - 1) & 0x000F0000 >> 16);
    tss_desc.type = 9;
    tss_desc.s = 0;
    tss_desc.dpl = 0;
    tss_desc.p = 1;
    tss_desc.avl = 0;
    tss_desc.l = 0;
    tss_desc.d_b = 0;
    tss_desc.g = 1;
    *((struct GDT_DESC*)(GDT_START_ADDR + 0x20)) = tss_desc;

    /* 用户代码段描述符 */
    struct GDT_DESC usrcode_desc;
    uint32_t usrcode_desc_size = sizeof(usrcode_desc);
    memset(&usrcode_desc, 0, usrcode_desc_size);
    usrcode_desc.base_0_15 = 0;
    usrcode_desc.base_16_23 = 0;
    usrcode_desc.base_24_31 = 0;
    usrcode_desc.limit_0_15 = 0xFFFF;
    usrcode_desc.limit_16_19 =0xF;
    usrcode_desc.type = 8;
    usrcode_desc.s = 1;
    usrcode_desc.dpl = 3;
    usrcode_desc.p = 1;
    usrcode_desc.avl = 0;
    usrcode_desc.l = 0;
    usrcode_desc.d_b = 1;
    usrcode_desc.g = 1;
    *((struct GDT_DESC*)(GDT_START_ADDR + 0x28)) = usrcode_desc;
    /* 用户数据段描述符 */
    struct GDT_DESC usrdata_desc;
    uint32_t usrdata_desc_size = sizeof(usrdata_desc);
    memset(&usrdata_desc, 0, usrdata_desc_size);
    usrdata_desc.base_0_15 = 0;
    usrdata_desc.base_16_23 = 0;
    usrdata_desc.base_24_31 = 0;
    usrdata_desc.limit_0_15 = 0xFFFF;
    usrdata_desc.limit_16_19 =0xF;
    usrdata_desc.type = 2;
    usrdata_desc.s = 1;
    usrdata_desc.dpl = 3;
    usrdata_desc.p = 1;
    usrdata_desc.avl = 0;
    usrdata_desc.l = 0;
    usrdata_desc.d_b = 1;
    usrdata_desc.g = 1;
    *((struct GDT_DESC*)(GDT_START_ADDR + 0x30)) = usrdata_desc;

    uint64_t gdt_ptr = ((8 * 7 - 1) | (((uint64_t)GDT_START_ADDR) << 16));
    asm volatile ("lgdt %0" : : "m" (gdt_ptr));
    asm volatile ("ltr %w0" : : "r" (SL_TSS));

    put_str("tss init done!\n");
}