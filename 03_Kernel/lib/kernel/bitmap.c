#include "bitmap.h"
#include "debug.h"
#include "string.h"
#include "interrupt.h"


/* 位图初始化 */
void bitmap_init(struct bitmap* bmp)
{
    memset(bmp->bits, 0, bmp->bytes_num);
}

/* 位图判断第index位的值：为 1 返回 true；为 0 返回 false */
bool bitmap_check(struct bitmap* bmp, uint32_t index)
{
    ASSERT(bmp->bits != NULL);
    uint32_t byte_idx = index / 8;
    uint32_t bit_idx = index % 8;
    ASSERT(byte_idx < bmp->bytes_num);
    if (bmp->bits[byte_idx] & (1 << bit_idx)) {
        return true;
    }
    return false;
}

/* 在位图中申请连续的cnt个bit位，成功则返回其其实的下标，失败返回 -1 */
int bitmap_apply(struct bitmap* bmp, uint32_t cnt)
{
    if (bmp->bits == NULL)
    {
        return -1;
    }

    uint32_t byte_idx = 0;
    while ((bmp->bits[byte_idx] == 0xFF) && (byte_idx < bmp->bytes_num)) {
        byte_idx++;
    }
    if (byte_idx >= bmp->bytes_num)
    {
        return -1;
    }

    uint32_t bit_idx = 0;
    while (bmp->bits[byte_idx] & (uint8_t)(1 << bit_idx)) {
        bit_idx++;
    }

    uint32_t index = byte_idx * 8 + bit_idx;
    uint32_t c = 0;
    while (index < (bmp->bytes_num * 8 - cnt)) {
        int i = index;
        for (i = 0; i < cnt; i++) {  // 判断是否 cnt 个连续的 bit 位可用
            if (bitmap_check(bmp, i)) {  // 返回 true 表示已被占用，寻找下一区间
                break;
            }
        }
        if (i == cnt) {
            return index;
        } else {
            index = i + 1;
        }
    }

    return -1;
}

/* 位图设置第index位的值 */
void bitmap_set(struct bitmap* bmp, uint32_t index, uint8_t val)
{
    ASSERT(bmp != NULL);
    ASSERT((val == 0) || (val == 1));
    uint32_t byte_idx = index / 8;
    uint32_t bit_idx = index % 8;

    if (val == 1) {
        bmp->bits[byte_idx] |= ((uint8_t)(1 << bit_idx));
    } else {
        bmp->bits[byte_idx] &= ~((uint8_t)(1 << bit_idx));
    }
}