#ifndef _LIB_KERNEL_LIST_H_
#define _LIB_KERNEL_LIST_H_

/* 链表节点 */
struct list_elem {
    struct list_elem* prev;  // 前躯结点
    struct list_elem* next;  // 后继结点
};

#endif
