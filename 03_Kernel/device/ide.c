#include "ide.h"
#include "stdint.h"
#include "stdio_kernel.h"
#include "lock.h"
#include "interrupt.h"
#include "stdio.h"
#include "io.h"
#include "semaphore.h"

/* 硬盘 device 寄存器结构 */
struct register_device {
    uint8_t lba_23_37 : 4;  // lba地址的23~27位
    uint8_t dev : 1;        // 主盘从盘标记：0为主盘，1为从盘
    uint8_t mbs1 : 1;       // MBS1：必须为1
    uint8_t mod : 1;        // 寻址模式：0为CHS，1为LBA
    uint8_t mbs2 : 1;       // MBS2：必须为1
};

/* 硬盘 status 寄存器结构 */
struct register_status {
    uint8_t err : 1;   // 错误状态标记
    uint8_t rev1 : 2;
    uint8_t drq : 1;   // 硬盘数据准备就绪状态标记
    uint8_t rev2 : 2;
    uint8_t drdy : 1;  //设备就绪标记
    uint8_t bsy : 1;   //设备繁忙标记
};

/* 硬盘各寄存器的端口号 */
#define reg_data(channel)        (channel->port_base + 0)
#define reg_error(channel)       (channel->port_base + 1)
#define reg_sector_cnt(channel)  (channel->port_base + 2)
#define reg_lba_low(channel)     (channel->port_base + 3)
#define reg_lba_mid(channel)     (channel->port_base + 4)
#define reg_lba_high(channel)    (channel->port_base + 5)
#define reg_device(channel)      (channel->port_base + 6)
#define reg_status(channel)      (channel->port_base + 7)
#define reg_command(channel)     (reg_status(channel))
#define reg_alt_status(channel)  (channel->port_base + 0x206)
#define reg_control(channel)     (reg_alt_status(channel))

/* 硬盘操作指令 */
#define CMD_IDENTIFY     (0xEC)  // 硬盘识别指令
#define CMD_READ_SECTOR  (0x20)  // 硬盘读扇区指令
#define CMD_WRITE_SECTOR (0x30)  // 硬盘写扇区指令

/* 选择读写的硬盘 */
static void select_disk(struct disk* hd) {
    struct register_device device = { 0 };
    device.mbs1 = 1;
    device.mbs2 = 1;
    device.mod = 1;  // LBA模式
    if (hd->dev_no == 1) {
        device.dev = 1;  // 从盘标记
    }
    outb(reg_dev(hd->my_channel), *(uint8_t*)(&device));
}

/* 向通道channel发命令cmd */
static void cmd_out(struct ide_channel* channel, uint8_t cmd) {
    /* 只要向硬盘发出了命令便将此标记置为true, 硬盘中断处理程序需要根据它来判断 */
    channel->expecting_intr = true;
    outb(reg_command(channel), cmd);
}

/* 获得硬盘参数信息 */
static void identify_disk(struct disk* hd) {
    char id_info[512];
    select_disk(hd);
    cmd_out(hd->my_channel, CMD_IDENTIFY);
    /* 向硬盘发送指令后便通过信号量阻塞自己, 待硬盘处理完成后,通过中断处理程序将自己唤醒 */
    semaphore_P(&hd->my_channel->disk_done);

    /* 醒来后开始执行下面代码 */
    if (!busy_wait(hd)) {     //  若失败
        char error[64];
        sprintf(error, "%s identify failed!!!!!!\n", hd->name);
        PANIC(error);
    }
    read_from_sector(hd, id_info, 1);

   char buf[64];
   uint8_t sn_start = 10 * 2, sn_len = 20, md_start = 27 * 2, md_len = 40;
   swap_pairs_bytes(&id_info[sn_start], buf, sn_len);
   printk("   disk %s info:\n      SN: %s\n", hd->name, buf);
   memset(buf, 0, sizeof(buf));
   swap_pairs_bytes(&id_info[md_start], buf, md_len);
   printk("      MODULE: %s\n", buf);
   uint32_t sectors = *(uint32_t*)&id_info[60 * 2];
   printk("      SECTORS: %d\n", sectors);
   printk("      CAPACITY: %dMB\n", sectors * 512 / 1024 / 1024);
}

/* 硬盘数据结构初始化 */
void ide_init() {
    printk("ide_init start\n");
    uint8_t hd_cnt = *((uint8_t*)(0x475));  // 获取硬盘的数量
    ASSERT(hd_cnt > 0);
   list_init(&partition_list);
    uint8_t channel_cnt = (hd_cnt + 2 - 1) / 2;  // 一个ide通道上有两个硬盘,根据硬盘数量反推有几个ide通道
    uint8_t channel_no = 0;
    uint8_t dev_no = 0;
    struct ide_channel* channel;

    /* 处理每个通道上的硬盘 */
    while (channel_no < channel_cnt) {
        channel = &channels[channel_no];
        sprintf(channel->name, "ide%d", channel_no);

        /* 为每个ide通道初始化端口基址及中断向量 */
        switch (channel_no) {
            case 0:
                channel->port_base = 0x1f0;   // ide0通道的起始端口号是0x1f0
                channel->irq_no = 0x20 + 14;  // 从片8259a上倒数第二的中断引脚,温盘,也就是ide0通道的的中断向量号
                break;
            case 1:
                channel->port_base = 0x170;   // ide1通道的起始端口号是0x170
                channel->irq_no = 0x20 + 15;  // 从8259A上的最后一个中断引脚,我们用来响应ide1通道上的硬盘中断
                break;
        }

        channel->expecting_intr = false;  // 未向硬盘写入指令时不期待硬盘的中断
        lock_init(&(channel->lock));		     

        /* 初始化信号量为0, 目的是向硬盘控制器请求数据后, 硬盘驱动sema_down此信号量会阻塞线程,
           直到硬盘完成后通过发中断, 由中断处理程序将此信号量sema_up, 唤醒线程。 */
        semaphore_init(&(channel->disk_done), 0);

        register_interrupt_handle(channel->irq_no, intr_hd_handler);  // 注册中断处理 handle

        /* 分别获取通道中两个硬盘的参数及分区信息 */
        while (dev_no < 2) {
            struct disk* hd = &(channel->devices[dev_no]);
            hd->my_channel = channel;
            hd->dev_no = dev_no;
            sprintf(hd->name, "sd%c", 'a' + channel_no * 2 + dev_no);
            identify_disk(hd);  // 获取硬盘参数
            if (dev_no != 0) {  // 内核本身的裸硬盘(hd60M.img)不处理
                partition_scan(hd, 0);  // 扫描该硬盘上的分区  
            }
    p_no = 0, l_no = 0;
            dev_no++; 
        }
        dev_no = 0;			  	   // 将硬盘驱动器号置0,为下一个channel的两个硬盘初始化。
        channel_no++;				   // 下一个channel
    }

   printk("\n   all partition info\n");
   /* 打印所有分区信息 */
   list_traversal(&partition_list, partition_info, (int)NULL);
   printk("ide_init done\n");
}