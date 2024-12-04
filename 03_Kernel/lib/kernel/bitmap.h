#ifndef _LIB_KERNEL_BITMAP_H_
#define _LIB_KERNEL_BITMAP_H_

#include "stdint.h"

struct bitmap {
    uint32_t bytes_num;  // 位图的字节数
    uint8_t* bits;       // 位图对应的其实地址
};

/* 位图初始化 */
void bitmap_init(struct bitmap* bmp);
/* 位图判断第index位的值：为 1 返回 true；为 0 返回 false */
bool bitmap_check(struct bitmap* bmp, uint32_t index);
/* 在位图中申请连续的cnt个bit位，成功则返回其其实的下标，失败返回 -1 */
int bitmap_apply(struct bitmap* bmp, uint32_t cnt);
/* 位图设置第index位的值 */
void bitmap_set(struct bitmap* bmp, uint32_t index, uint8_t val);

#endif