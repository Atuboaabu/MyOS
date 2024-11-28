#ifndef _LIB_IO_H_
#define _LIB_IO_H_

#include "stdint.h"

/* 向端口号为port的端口写入一个字节 */
inline void outb(uint16_t port, uint8_t data);
/* 从端口号为port的端口读入一个字节 */
inline uint8_t inb(uint16_t port);

/* 将addr起的n个字节写入端口号为port的端口 */
inline void outsw(uint16_t port, const void* addr, uint32_t n);
/* 从端口号为port的端口读入n个字节到addr */
inline void insw(uint16_t port, uint32_t n, void* addr);

#endif