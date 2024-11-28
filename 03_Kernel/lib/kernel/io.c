#include "io.h"

/* a表示用寄存器al或ax或eax
* 对端口指定N表示0~255, d表示用dx存储端口号,
* %b0表示对应al,%w1表示对应dx */ 
inline void outb(uint16_t port, uint8_t data)
{
    asm volatile ("outb %b0, %w1" : : "a" (data), "Nd" (port));    
}

inline uint8_t inb(uint16_t port)
{
    uint8_t data;
    asm volatile ("inb %w1, %b0" : "=a" (data) : "Nd" (port));
    return data;
}

/* +表示此限制即做输入又做输出.
** outsw是把ds:esi处的16位的内容写入port端口, 我们在设置段描述符时, 
** 已经将ds,es,ss段的选择子都设置为相同的值了, 此时不用担心数据错乱。*/
inline void outsw(uint16_t port, const void* addr, uint32_t n)
{
    asm volatile ("cld; rep outsw" : "+S" (addr), "+c" (n) : "d" (port));
}

/* insw是将从端口port处读入的16位内容写入es:edi指向的内存
** 我们在设置段描述符时, 已经将ds,es,ss段的选择子都设置为相同的值了,
** 此时不用担心数据错乱。*/
inline void insw(uint16_t port, uint32_t n, void* addr)
{
    asm volatile ("cld; rep insw" : "+D" (addr), "+c" (n) : "d" (port) : "memory");
}