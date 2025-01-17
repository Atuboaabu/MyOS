#include "ioqueue.h"
#include "lock.h"

#define NEXT_INDEX(index) ((index + 1) % IO_QUEUE_BUFFER_SIZE)

void ioqueue_putchar(struct ioqueue* p_ioqueue, char c) {
    lock_acquire(&(p_ioqueue->lock));
    while (NEXT_INDEX(p_ioqueue->head_index) == p_ioqueue->tail_index) {  // 缓冲区满
        condition_wait(&(p_ioqueue->full_cond), &(p_ioqueue->lock));
    }
    p_ioqueue->buffer[p_ioqueue->head_index] = c;
    p_ioqueue->head_index = NEXT_INDEX(p_ioqueue->head_index);
    lock_release(&(p_ioqueue->lock));
    condition_notify(&(p_ioqueue->empty_cond));
}

char ioqueue_getchar(struct ioqueue* p_ioqueue) {
    lock_acquire(&(p_ioqueue->lock));
    while (p_ioqueue->head_index == p_ioqueue->tail_index) {  // 缓冲区空
        condition_wait(&(p_ioqueue->empty_cond), &(p_ioqueue->lock));
    }
    char c = p_ioqueue->buffer[p_ioqueue->tail_index];
    p_ioqueue->tail_index = NEXT_INDEX(p_ioqueue->tail_index);
    lock_release(&(p_ioqueue->lock));
    condition_notify(&(p_ioqueue->full_cond));
    return c;
}

void ioqueue_init(struct ioqueue* p_ioqueue) {
    p_ioqueue->head_index = 0;
    p_ioqueue->tail_index = 0;
    lock_init(&(p_ioqueue->lock));
    condition_init(&(p_ioqueue->empty_cond));
    condition_init(&(p_ioqueue->full_cond));
}