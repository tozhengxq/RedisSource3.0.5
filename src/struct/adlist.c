/* adlist.c - A generic doubly linked list implementation
 *
 * Copyright (c) 2006-2010, Salvatore Sanfilippo <antirez at gmail dot com>
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


#include <stdlib.h>
#include "adlist.h"
#include "zmalloc.h"

/* Create a new list. The created list can be freed with
 * AlFreeList(), but private value of every node need to be freed
 * by the user before to call AlFreeList().
 *
 * On error, NULL is returned. Otherwise the pointer to the new list. */
// list链表可以被AlFreeList释放掉，但是具体的结点的value需要事先释放掉，才能调用这个函数

/*
* 初始化一个空链表，并且申请相应的内存，失败则返回null
* 将链表的头结点和尾节点均指向null （空结点）
* 将变量len和函数指针free,match 以及 dup均赋值为null */
list *listCreate(void)
{
    struct list *list;

    if ((list = zmalloc(sizeof(*list))) == NULL)
        return NULL;
    list->head = list->tail = NULL;
    list->len = 0;
    list->dup = NULL;
    list->free = NULL;
    list->match = NULL;
    return list;
}

/* Free the whole list.
 *
 * This function can't fail. */

/* 定义释放链表的函数，传进来的是一个链表的地址 */

void listRelease(list *list)
{
    unsigned long len;
    listNode *current, *next;  // 预定义两个链表结点

    current = list->head; // 将头结点赋值给current，表示释放从头结点开始
    len = list->len; // 将链表长度赋值给len
    while(len--) { // 循环逐个释放链表中的结点
        next = current->next; // 指向下一个
        if (list->free) list->free(current->value); // 调用函数指针free，前面listCreate函数中会给函数指针free指定一个函数，这里直接调用，就是如果定义了话，不是null，就会调用自己的free函数来释放，将链表节点的value传进来。
        zfree(current); // 调用zmalloc.c zmalloc.h 中定义的zfree来释放，并不是free
        current = next; // 结合next＝current->next 来实现循环
    }
    zfree(list); // 在释放了所有链表结点以后，再释放链表，同样是调用zfree
}

/* Add a new node to the list, to head, containing the specified 'value'
 * pointer as value.
 *
 * On error, NULL is returned and no operation is performed (i.e. the
 * list remains unaltered).
 * On success the 'list' pointer you pass to the function is returned. */

/* 向一个已存在的链表中增加链表结点，且是头结点
*  传进来的参数就是相应链表和对应的结点存储的值 */

list *listAddNodeHead(list *list, void *value)
{
    listNode *node; // 初始化一个结点

    if ((node = zmalloc(sizeof(*node))) == NULL)
        return NULL;  // 会预先判断能否成功申请到内存，并不是实际申请。
    node->value = value; // 对结点中的value赋值
    //  加入头结点，自然头结点的prev要指向null，如果该链表还是个空链表的话，那么该新加的节点就既是头结点也是尾节点
    if (list->len == 0) { 
        list->head = list->tail = node;
        node->prev = node->next = NULL;
    } else {
        node->prev = NULL;
        node->next = list->head;
        list->head->prev = node;
        list->head = node;
    }
    list->len++;  // 链表长度加1
    return list;
}


/* Add a new node to the list, to tail, containing the specified 'value'
 * pointer as value.
 *
 * On error, NULL is returned and no operation is performed (i.e. the
 * list remains unaltered).
 * On success the 'list' pointer you pass to the function is returned. */

/* 添加尾节点，道理同添加头结点相同 */

list *listAddNodeTail(list *list, void *value)
{
    listNode *node;

    if ((node = zmalloc(sizeof(*node))) == NULL)
        return NULL;
    node->value = value;
    if (list->len == 0) {
        list->head = list->tail = node;
        node->prev = node->next = NULL;
    } else {
        node->prev = list->tail;
        node->next = NULL;
        list->tail->next = node;
        list->tail = node;
    }
    list->len++;
    return list;
}

/* 新增一个普通结点 道理和新增头结点，尾结点类似 这里不再赘述*/
list *listInsertNode(list *list, listNode *old_node, void *value, int after) {
    listNode *node;

    if ((node = zmalloc(sizeof(*node))) == NULL)
        return NULL;
    node->value = value;
    if (after) {
        node->prev = old_node;
        node->next = old_node->next;
        if (list->tail == old_node) {
            list->tail = node;
        }
    } else {
        node->next = old_node;
        node->prev = old_node->prev;
        if (list->head == old_node) {
            list->head = node;
        }
    }
// 这里对新增节点的前后链接是否正常，做进一步检查并重新赋一次值。
    if (node->prev != NULL) {
        node->prev->next = node;
    }
    if (node->next != NULL) {
        node->next->prev = node;
    }
    list->len++;
    return list;
}

/* Remove the specified node from the specified list.
 * It's up to the caller to free the private value of the node.
 *
 * This function can't fail. */

/* 删除指定的结点 
*  原理就是，将该结点的前一个结点的next指向该结点的下一个结点 node->prev->next = node->next
*  将该结点的后一个结点的prev指向该结点的前一个节点 node->next->prev = node->prev
*  注意 不管删结点还是新增结点，在牵扯到头结点和尾结点的操作的时候，都有去更新相应的链表*/

void listDelNode(list *list, listNode *node)
{
    if (node->prev)
        node->prev->next = node->next;
    else
        list->head = node->next;
    if (node->next)
        node->next->prev = node->prev;
    else
        list->tail = node->prev;
    if (list->free) list->free(node->value);
    zfree(node);
    list->len--;
}

/* Returns a list iterator 'iter'. After the initialization every
 * call to listNext() will return the next element of the list.
 *
 * This function can't fail. */

/* 创建一个链表的迭代器，该操作不会失败 
*  adlist.h 中定义 AL_START_HEAD 0, AL_START_TAIL 1 
*  该迭代器，要不从头开始，要不从尾开始*/

listIter *listGetIterator(list *list, int direction)
{
    listIter *iter;

    if ((iter = zmalloc(sizeof(*iter))) == NULL) return NULL;
    if (direction == AL_START_HEAD)
        iter->next = list->head;
    else
        iter->next = list->tail;
    iter->direction = direction;
    return iter;
}


/* Release the iterator memory */
/* 如题 释放迭代器，调用的是zfree */
void listReleaseIterator(listIter *iter) {
    zfree(iter);
}

/* Create an iterator in the list private iterator structure */
/* 将迭代器重置为从头开始*/
void listRewind(list *list, listIter *li) {
    li->next = list->head;
    li->direction = AL_START_HEAD;
}

/* 将迭代器重置为从尾开始*/
void listRewindTail(list *list, listIter *li) {
    li->next = list->tail;
    li->direction = AL_START_TAIL;
}

/* Return the next element of an iterator.
 * It's valid to remove the currently returned element using
 * listDelNode(), but not to remove other elements.
 *
 * The function returns a pointer to the next element of the list,
 * or NULL if there are no more elements, so the classical usage patter
 * is:
 *
 * iter = listGetIterator(list,<direction>);
 * while ((node = listNext(iter)) != NULL) {
 *     doSomethingWith(listNodeValue(node));
 * }
 *
 * */

/* 根据迭代器的定义（链表和方向），对链表进行循环，返回当前的链表结点*/
listNode *listNext(listIter *iter)
{
    listNode *current = iter->next;
    // 定义迭代器时候，iter->next 要不是头结点，要不是尾节点，如果为空，则链表为空
    if (current != NULL) {
        if (iter->direction == AL_START_HEAD)
            iter->next = current->next;
        else
            iter->next = current->prev;
    }
    return current;
}

/* Duplicate the whole list. On out of memory NULL is returned.
 * On success a copy of the original list is returned.
 *
 * The 'Dup' method set with listSetDupMethod() function is used
 * to copy the node value. Otherwise the same pointer value of
 * the original node is used as value of the copied node.
 *
 * The original list both on success or error is never modified. */
/* 复制一个链表
*  原理：利用迭代器，逐个将链表结点复制到新的链表中，传入参数是原始链表 */

list *listDup(list *orig)
{
    list *copy;
    listIter *iter;
    listNode *node;

    if ((copy = listCreate()) == NULL)
        return NULL; // 如果新链表创建失败，则直接返回null
    copy->dup = orig->dup;
    copy->free = orig->free;
    copy->match = orig->match;

    iter = listGetIterator(orig, AL_START_HEAD); // 定义一个原链表的从头开始的迭代器
    // 迭代下一个链表结点给node，如果链表有自定义的dup函数则优先使用，否则直接赋值给copy链表
    while((node = listNext(iter)) != NULL) {
        void *value; 

        if (copy->dup) {
            value = copy->dup(node->value);
            if (value == NULL) {
                listRelease(copy);
                listReleaseIterator(iter);
                return NULL;
            }
        } else
            value = node->value;
        if (listAddNodeTail(copy, value) == NULL) {
	   // 后面的结点都是一直添加到新链表的尾部，如果内存溢出，则释放copy和orig的迭代器
	    listRelease(copy);
            listReleaseIterator(iter);
            return NULL;
       }
    }
    listReleaseIterator(iter);
    return copy;
}

/* Search the list for a node matching a given key.
 * The match is performed using the 'match' method
 * set with listSetMatchMethod(). If no 'match' method
 * is set, the 'value' pointer of every node is directly
 * compared with the 'key' pointer.
 *
 * On success the first matching node pointer is returned
 * (search starts from head). If no matching node exists
 * NULL is returned. */

/* 通过迭代器从头部对链表进行逐个比对，直到匹配到相应的value，否则遍历完毕返回null */

listNode *listSearchKey(list *list, void *key)
{
    listIter *iter;
    listNode *node;

    iter = listGetIterator(list, AL_START_HEAD);
    while((node = listNext(iter)) != NULL) {
        if (list->match) {
            if (list->match(node->value, key)) {
                listReleaseIterator(iter);
                return node;
            }
        } else {
            if (key == node->value) {
                listReleaseIterator(iter);
                return node;
            }
        }
    }
    listReleaseIterator(iter);
    return NULL;
}

/* Return the element at the specified zero-based index
 * where 0 is the head, 1 is the element next to head
 * and so on. Negative integers are used in order to count
 * from the tail, -1 is the last element, -2 the penultimate
 * and so on. If the index is out of range NULL is returned. */

/* 根据索引下标来返回相应的节点
*  下标是正数：从0开始即  0 1 2 4 ...  一次指向下一个结点
*  下标是负数：从-1开始 即 -1 -2 -3 -4 ..  依次指向前一个结点
*/
listNode *listIndex(list *list, long index) {
    listNode *n;

    if (index < 0) {
        index = (-index)-1;
        n = list->tail;
        while(index-- && n) n = n->prev;
    } else {
        n = list->head; // n 赋为头节点开始
        while(index-- && n) n = n->next; 
	// 当index--大于0 和 n都成立的时候，说明index在链表范围内，则继续 n->next
        // 当index--大于0 不成立，n成立，说明index遍历完了，且在链表范围内，此时的index应为0,即为上一次循环的输出值，此次条件不成立
	//  就不在执行n=n-->next ，所以返回的是index为0，而n则是null，因为链表下标从0开始，比如 index为5，则输出的index值为 4 3
	// 2 1 0,所以对应的应该是下标为0 1 2 3 4，对应的是下标为4的结点，但是这里有n=n-->next 所以返回的是下标为5的结点
	// 当index--大于0成立，n不成立，说明idnex out of range了，此时n为null
	// 所以要不返回相应的索引下标的结点，要不返回null
    }
    return n;
}

/* Rotate the list removing the tail node and inserting it to the head. */
/* 反转链表，这里定义了一个操作，就是 将尾节点放到头结点的前面 
*  如果要反转链表，应该在一个链表长度的循环里去循环调用这个函数，*/

void listRotate(list *list) {
    listNode *tail = list->tail;

    if (listLength(list) <= 1) return;

    /* Detach current tail */
    list->tail = tail->prev;
    list->tail->next = NULL;
    /* Move it as head */
    list->head->prev = tail;
    tail->prev = NULL;
    tail->next = list->head;
    list->head = tail;
}
