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

char* strcpy(char* dst, const char* src) {
   ASSERT(dst != NULL && src != NULL);
   char* p = dst;
   while((*dst++ = *src++));
   return p;
}