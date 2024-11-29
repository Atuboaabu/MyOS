#ifndef _LIB_IO_H_
#define _LIB_IO_H_

#include "stdint.h"

/* 向端口号为port的端口写入一个字节 */
static inline void outb(uint16_t port, uint8_t data)
{
    asm volatile ("outb %b0, %w1" : : "a" (data), "Nd" (port));    
}
/* 从端口号为port的端口读入一个字节 */
static inline uint8_t inb(uint16_t port)
{
    uint8_t data;
    asm volatile ("inb %w1, %b0" : "=a" (data) : "Nd" (port));
    return data;
}

/* 将addr起的n个字节写入端口号为port的端口 */
static inline void outsw(uint16_t port, const void* addr, uint32_t n)
{
    asm volatile ("cld; rep outsw" : "+S" (addr), "+c" (n) : "d" (port));
}
/* 从端口号为port的端口读入n个字节到addr */
static inline void insw(uint16_t port, uint32_t n, void* addr)
{
    asm volatile ("cld; rep insw" : "+D" (addr), "+c" (n) : "d" (port) : "memory");
}

#endif