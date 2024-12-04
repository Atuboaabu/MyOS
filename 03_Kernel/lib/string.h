#ifndef _LIB_STRING_H_
#define _LIB_STRING_H_
#include "stdint.h"

#define NULL ((void*)0)
/* 从 dst 地址起将 size 个字节的值置为 val */
void memset(void* dst, uint8_t val, uint32_t size);

#endif