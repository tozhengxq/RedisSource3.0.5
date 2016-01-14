/* adlist.h - A generic doubly linked list implementation
 *
 * Copyright (c) 2006-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __ADLIST_H__
#define __ADLIST_H__

/* Node, List, and Iterator are the only data structures used currently. */
// 定义链表的节点结构
typedef struct listNode {
    struct listNode *prev; // 定义指向前一节点的指针
    struct listNode *next; // 定义指向下一节点的指针
    void *value; // 节点的值
} listNode;

// 定义链表的迭代器
typedef struct listIter {
    listNode *next; // 指向下一节点
    int direction; // 迭代器的方向
} listIter;

// 定义的链表结构，来管理该双向链表的节点
// 该双向链表不是闭环的，头节点的的*next 和尾节点的*prev 均指向null
typedef struct list {
    listNode *head; // 链表头节点
    listNode *tail; // 链表尾节点
    void *(*dup)(void *ptr); // 定义函数指针dup，并套一个指针函数，返回未知类型的指针；复制
    void (*free)(void *ptr); // 定义函数指针free；释放
    int (*match)(void *ptr, void *key); // 定义函数指针match；匹配
    unsigned long len; // 定义一个len变量，存储双向链表的长度，应该是链表节点个数
} list;

/* Functions implemented as macros */
// 宏定义函数
// 如第一个：#define listLength(l) ((l)->len)
// 定义一个函数listLength(l)，操作就是返回 (l)->len ，就是返回结构体的len

#define listLength(l) ((l)->len)
#define listFirst(l) ((l)->head)
#define listLast(l) ((l)->tail)
#define listPrevNode(n) ((n)->prev)
#define listNextNode(n) ((n)->next)
#define listNodeValue(n) ((n)->value)

#define listSetDupMethod(l,m) ((l)->dup = (m))
#define listSetFreeMethod(l,m) ((l)->free = (m))
#define listSetMatchMethod(l,m) ((l)->match = (m))

#define listGetDupMethod(l) ((l)->dup)
#define listGetFree(l) ((l)->free)
#define listGetMatchMethod(l) ((l)->match)

/* Prototypes */
// 函数定义
list *listCreate(void); // 定义一个指针函数，创建一个新的链表，返回的指针类型就是前面定义的list
			// 也就是返回新建链表的在内存中的地址
void listRelease(list *list); // 释放链表的函数
list *listAddNodeHead(list *list, void *value); // 指针函数，增加链表头节点
list *listAddNodeTail(list *list, void *value); // 指针函数，增加链表尾节点
list *listInsertNode(list *list, listNode *old_node, void *value, int after); // 指针函数，向链表中增加节点，返回最新的链表结构。
void listDelNode(list *list, listNode *node); // 从链表中删除节点，？为什么不返回新的链表结构，可能是删除节点暂时不会释放内存？
listIter *listGetIterator(list *list, int direction); // 返回某个链表的迭代器
listNode *listNext(listIter *iter); // 传进来迭代器，返回链表的下一个节点，！应该是用作查询链表时候用，而且是双向的
void listReleaseIterator(listIter *iter); // 释放某个链表的迭代器
list *listDup(list *orig); // 复制一个链表，返回最新的那个
listNode *listSearchKey(list *list, void *key); // 链表查询，返回匹配到的那个节点
listNode *listIndex(list *list, long index); // 链表的索引，从头部开始 从0开始计数，如果是负的，则是从尾部开始，即－1表示最后一个节点
void listRewind(list *list, listIter *li); // 重置链表，从头开始
void listRewindTail(list *list, listIter *li); // 重置链表，从尾开始
void listRotate(list *list); // 旋转链表

//
// 以上函数定义的具体实现在adlist.c 中
//
/* Directions for iterators */
#define AL_START_HEAD 0
#define AL_START_TAIL 1

#endif /* __ADLIST_H__ */
