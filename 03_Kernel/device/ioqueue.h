#ifndef _DEVICE_IOQUEUE_H_
#define _DEVICE_IOQUEUE_H_

#include "stdint.h"
#include "thread.h"
#include "lock.h"
#include "condition_variable.h"

/* 缓冲区大小 */
#define IO_QUEUE_BUFFER_SIZE (64)

struct ioqueue {
    struct lock lock;                     // 环形队列互斥锁
    char buffer[IO_QUEUE_BUFFER_SIZE];    // 缓冲区
    uint32_t head_index;                  // 队首索引
    uint32_t tail_index;                  // 队尾索引
    struct condition_variable empty_cond; // 队列空
    struct condition_variable full_cond;  // 队列满
};

/* 初始化io队列 */
void ioqueue_init(struct ioqueue* p_ioqueue);
/* 从队列中获取字符 */
char ioqueue_getchar(struct ioqueue* p_ioqueue);
/* 向队列中写入字符 */
void ioqueue_putchar(struct ioqueue* p_ioqueue, char c);

#endif