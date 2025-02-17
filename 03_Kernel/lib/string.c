#include "string.h"
#include "debug.h"

void memset(void* dst, uint8_t val, uint32_t size)
{
    ASSERT(dst != NULL);
    ASSERT(size > 0);
    uint8_t* p = (uint8_t*)dst;
    while(size-- > 0) {
        *p++ = val;
    }
}

void memcpy(void* dst, const void* src, uint32_t size) {
    ASSERT(dst != NULL && src != NULL);
    uint8_t* _dst = dst;
    const uint8_t* _src = src;
    while (size-- > 0) {
        *_dst++ = *_src++;
    }
}

char* strcpy(char* dst, const char* src) {
   ASSERT(dst != NULL && src != NULL);
   char* p = dst;
   while((*dst++ = *src++));
   return p;
}

int8_t strcmp (const char* str1, const char* str2) {
   ASSERT(str1 != NULL && str2 != NULL);
   while (*str1 != 0 && *str1 == *str2) {
      str1++;
      str2++;
   }
   return *str1 < *str2 ? -1 : *str1 > *str2;
}

uint32_t strlen(const char* str) {
    ASSERT(str != NULL);
    const char* p = str;
    while(*p++);
    return (p - str - 1);
}