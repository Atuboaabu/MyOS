#ifndef _LIB_STRING_H_
#define _LIB_STRING_H_
#include "stdint.h"

/* 从 dst 地址起将 size 个字节的值置为 val */
void memset(void* dst, uint8_t val, uint32_t size);
void memcpy(void* dst, const void* src, uint32_t size);
char* strcpy(char* dst, const char* src);

#endif