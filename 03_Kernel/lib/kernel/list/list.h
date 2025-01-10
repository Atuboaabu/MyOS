#ifndef _LIB_KERNEL_LIST_H_
#define _LIB_KERNEL_LIST_H_

#include "stdint.h"

/* 获取结构体成员的偏移量 */
#define GET_ENTRYMEMBER_OFFSET(entry_type, entry_member) \
        (uint32_t)(&((entry_type*)0)->entry_member)
/* 通过结构体中对应的链表的TAG结点获取结构体的地址 */
#define GET_ENTRYPTR_FROM_LISTTAG(entry_type, entry_member, tag_ptr) \
        (entry_type*)((uint32_t)tag_ptr - GET_ENTRYMEMBER_OFFSET(entry_type, entry_member))


/* 链表节点 */
struct list_elem {
    struct list_elem* prev;  // 前躯结点
    struct list_elem* next;  // 后继结点
};

/* 链表结构 */
struct list {
    /* head是队首, 是固定不变的, 不是第1个元素,第1个元素为head.next */
    struct list_elem head;
    /* tail是队尾, 同样是固定不变的, 不是最后一个元素, 最后一个元素为 tail.prev */
    struct list_elem tail;
};

/* 自定义函数类型 list_callback, 用于在list_traversal中做回调函数 */
typedef bool (list_callback)(struct list_elem*, int arg);

/* 链表初始化 */
void list_init(struct list* _list);
/* 插入元素到指定的before元素之前 */
void list_insert_before(struct list_elem* _before_elem, struct list_elem* _elem);
/* 插入元素到链表首端 */
void list_push(struct list* _list, struct list_elem* _elem);
/* 插入元素到链表尾端 */
void list_append(struct list* _list, struct list_elem* _elem);
/* 移除链表中的指定元素 */
void list_remove(struct list_elem* _elem);
/* 在链表中查找元素 */
bool elem_find(struct list* _list, struct list_elem* _elem);
/* 判断链表是否为空 */
bool list_empty(struct list* _list);
/* 链表长度 */
uint32_t list_len(struct list* _list);
/* 链表弹出首端元素 */
struct list_elem* list_pop(struct list* _list);
/* 找到链表中满足回调条件的元素 */
struct list_elem* list_traversal(struct list* _list, list_callback _callback, int _arg);

#endif
