#ifndef _LIB_STRING_H_
#define _LIB_STRING_H_
#include "stdint.h"

/* 从 dst 地址起将 size 个字节的值置为 val */
void memset(void* dst, uint8_t val, uint32_t size);
/* 从 dst 地址起将 size 个字节的值拷贝到 dst */
void memcpy(void* dst, const void* src, uint32_t size);
/* 连续比较以地址 ptr1 和地址 ptr2 开头的 size 个字节: 若相等则返回0, 若 ptr1 大于 ptr2 返回 1, 否则返回 -1 */
int memcmp(const void* ptr1, const void* ptr2, uint32_t size);
/* 复制字符串 src 到 dst */
char* strcpy(char* dst, const char* src);
/* 比较两个字符串,若 str1 中的字符大于 str2 中的字符返回1, 相等时返回0, 否则返回-1. */
int8_t strcmp (const char* str1, const char* str2);
/* 将字符串 _src 拼接到 _dst 后, 返回拼接的串地址 */
char* strcat(char* _dst, const char* _src);
/* 从前往后查找字符串 str 中首次出现字符 ch 的地址 */
char* strchr(const char* str, const char ch);
/* 从后往前查找字符串 str 中首次出现字符 ch 的地址 */
char* strrchr(const char* str, const char ch);
/* 字符串长度获取 */
uint32_t strlen(const char* str);

#endif