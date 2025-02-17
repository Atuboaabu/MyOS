#ifndef _LIB_STRING_H_
#define _LIB_STRING_H_
#include "stdint.h"

/* 从 dst 地址起将 size 个字节的值置为 val */
void memset(void* dst, uint8_t val, uint32_t size);
/* 从 dst 地址起将 size 个字节的值拷贝到 dst */
void memcpy(void* dst, const void* src, uint32_t size);
/* 复制字符串 src 到 dst */
char* strcpy(char* dst, const char* src);
/* 比较两个字符串,若 str1 中的字符大于 str2 中的字符返回1, 相等时返回0, 否则返回-1. */
int8_t strcmp (const char* str1, const char* str2);
/* 字符串长度获取 */
uint32_t strlen(const char* str);

#endif