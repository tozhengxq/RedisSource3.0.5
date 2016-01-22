/* Hash Tables Implementation.
 *
 * This file implements in-memory hash tables with insert/del/replace/find/
 * get-random-element operations. Hash tables will auto-resize if needed
 * tables of power of two in size are used, collisions are handled by
 * chaining. See the source code for more information... :)
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

/* redis中的数据类型，如hash哈希表，set集合底层都是用字典来实现的
*  字典的实现是通过哈希表的算法来实现的
*  其他：
*  当要添加一个键值对到字典里面时，程序需要先根据键值对的键计算出哈西值和索引值，然后再根据索引值将包含
*  新键值对的哈西节点放到哈西表数组的指定索引上面
*  利用哈希表 里的next，形成链表，来解决字典中的键冲突问题*/

#include <stdint.h>

#ifndef __DICT_H
#define __DICT_H

#define DICT_OK 0
#define DICT_ERR 1

/* Unused arguments generate annoying warnings... */
#define DICT_NOTUSED(V) ((void) V)

/*定义哈希表的节点，节点存储相应的键值对,上级对应着哈希表*/ 
typedef struct dictEntry {
    void *key; // 存储键
    union {
        void *val;
        uint64_t u64;
        int64_t s64;
        double d;
    } v; // 存储相应的值，可以是指针，uint64等
    struct dictEntry *next; // 一个指向next 节点的指针，用以解决字典的key（计算出hash值）的冲突问题
			    // key的hash值相同的键值对，会在这里形成一个链表，因为链表只有next指针，所以最后插入的节点排在链表
			    // 最前面，redis中，就相当于冲突的key不会报错，会覆盖掉原来的key
} dictEntry;

/* 字典类型，存储相关的操作函数，上级是字典*/
typedef struct dictType {
    unsigned int (*hashFunction)(const void *key); // 计算key的hash值的函数
    void *(*keyDup)(void *privdata, const void *key); // 
    void *(*valDup)(void *privdata, const void *obj);
    int (*keyCompare)(void *privdata, const void *key1, const void *key2);
    void (*keyDestructor)(void *privdata, void *key);
    void (*valDestructor)(void *privdata, void *obj);
} dictType;

/* This is our hash table structure. Every dictionary has two of this as we
 * implement incremental rehashing, for the old to the new table. */

/*哈希表，对应上级是字典dict*/

typedef struct dictht {
    dictEntry **table; // 定义哈希表中的哈希节点,节点指针数组，各个节点以数组的形式在hashtable中, dictEntry*[size]
    unsigned long size; // 哈希表大小，也就是哈希节点数量
    unsigned long sizemask; // mask 码，用以地址索引计算
    unsigned long used; // 哈希表size使用量，哈希节点的使用量
} dictht;

/*字典*/
typedef struct dict {
    dictType *type; // 字典类型，对应着一组操作函数
    void *privdata; // 私有数据
    dictht ht[2]; // 哈希表，这里一般有ht[0] ht[1] 两个哈希表，用以字典的渐进式rehash操作
    long rehashidx; 
/* rehashing not in progress if rehashidx == -1 ，字典rehash操作的
* 标志量 0 为开始尽行rehash，－1 为停止，在整个rehash过程中，rehashidx实际上是代表的
* 是dictentry的索引下标，所以在rehash期间，有新的操作进来的时候，redis会顺带将rehashidx
* 索引上的键值对rehash到ht[1] , 所以日积月累，就会rehash完毕，这也是渐进式reash*/
    int iterators; /* number of iterators currently running */
} dict;

/* If safe is set to 1 this is a safe iterator, that means, you can call
 * dictAdd, dictFind, and other functions against the dictionary even while
 * iterating. Otherwise it is a non safe iterator, and only dictNext()
 * should be called while iterating. */
/* 字典的迭代器*/
typedef struct dictIterator {
    dict *d; // 字典
    long index;
    int table, safe;
    dictEntry *entry, *nextEntry; // 哈希表节点指针，和指向下一个节点的指针
    /* unsafe iterator fingerprint for misuse detection. */
    long long fingerprint;
} dictIterator;

/* 接口定义，函数定义，对应的实现方法在dict.c 中*/


// 字典扫描方法
typedef void (dictScanFunction)(void *privdata, const dictEntry *de);

/* This is the initial size of every hash table */
#define DICT_HT_INITIAL_SIZE     4  
// hash表初始化的大小，4

/* ------------------------------- Macros ------------------------------------*/
/* 字典释放val时候调用，如果dictType有定义这个valDestructor函数的话，就调用这个函数*/
#define dictFreeVal(d, entry) \
    if ((d)->type->valDestructor) \
        (d)->type->valDestructor((d)->privdata, (entry)->v.val)
/* 字典设置val的时候调用，如果dictType有定义就用*/
#define dictSetVal(d, entry, _val_) do { \
    if ((d)->type->valDup) \
        entry->v.val = (d)->type->valDup((d)->privdata, _val_); \
    else \
        entry->v.val = (_val_); \
} while(0)
/* 接下来四个的定义，是对哈希节点dictEntry中union v的赋值，union定义类似与结构体，但是它们共用一块
*  内存，一次只能对其中一个赋值，对某一个成员赋值就会覆盖其他成员的值；union大小是其成员中最大的大小*/
// 设置dictEntry中共用体v的值
#define dictSetSignedIntegerVal(entry, _val_) \
    do { entry->v.s64 = _val_; } while(0)

#define dictSetUnsignedIntegerVal(entry, _val_) \
    do { entry->v.u64 = _val_; } while(0)

#define dictSetDoubleVal(entry, _val_) \
    do { entry->v.d = _val_; } while(0)
/*dictFreeKey,dictSetKey 与 dictFreeVal ,dictSetVal类似
* 如果dictType中有定义则使用*/
#define dictFreeKey(d, entry) \
    if ((d)->type->keyDestructor) \
        (d)->type->keyDestructor((d)->privdata, (entry)->key)

#define dictSetKey(d, entry, _key_) do { \
    if ((d)->type->keyDup) \
        entry->key = (d)->type->keyDup((d)->privdata, _key_); \
    else \
        entry->key = (_key_); \
} while(0)
/* 也是和dictType有关，定义两个key值的比较，如果dictType中有定义，则调用，没有则直接比较*/
#define dictCompareKeys(d, key1, key2) \
    (((d)->type->keyCompare) ? \
        (d)->type->keyCompare((d)->privdata, key1, key2) : \
        (key1) == (key2))
// 获取key的hash值，调用dictType中的hashFunction函数
#define dictHashKey(d, key) (d)->type->hashFunction(key)
// 获取哈希节点dictEntry 的key值
#define dictGetKey(he) ((he)->key)
// 获取dictEntry的共用体中的val值
#define dictGetVal(he) ((he)->v.val)
// ... 有符号值
#define dictGetSignedIntegerVal(he) ((he)->v.s64)
// ... 无符号值
#define dictGetUnsignedIntegerVal(he) ((he)->v.u64)
// ... double类型值
#define dictGetDoubleVal(he) ((he)->v.d)
// 获取字典大小，取哈西表1 和 2 的和
#define dictSlots(d) ((d)->ht[0].size+(d)->ht[1].size)
// 获取字典使用大小，去哈希表1 和 2 的used值的和
#define dictSize(d) ((d)->ht[0].used+(d)->ht[1].used)
// 初始化rehash标志，为  －1 ，置为0表示开始rehash
#define dictIsRehashing(d) ((d)->rehashidx != -1)

/* API */
// 字典创建，传入type和privadata  返回一个字典
dict *dictCreate(dictType *type, void *privDataPtr);
// 字典扩充
int dictExpand(dict *d, unsigned long size);
// 根据key 和 value 向字典中添加一个dictEntry
int dictAdd(dict *d, void *key, void *val);
// 向字典中添加一个key，没有value，返回指向该key所在的dictEntry
dictEntry *dictAddRaw(dict *d, void *key);
// 替换字典中的相应key value的dictEntry
int dictReplace(dict *d, void *key, void *val);
// 根据key替换字典中的dictEntry，返回指向该entry的指针
dictEntry *dictReplaceRaw(dict *d, void *key);
// 根据key删除一个dictEntry
int dictDelete(dict *d, const void *key);
// 根据key删除一个dictEntry，不释放？？？
int dictDeleteNoFree(dict *d, const void *key);
// 释放一个字典
void dictRelease(dict *d);
// 根据key查找一个dictEntry
dictEntry * dictFind(dict *d, const void *key);
// 根据key从字典中取出相应的value
void *dictFetchValue(dict *d, const void *key);
// 重定义dict的大小
int dictResize(dict *d);
// 创建某个字典的迭代器
dictIterator *dictGetIterator(dict *d);
// 创建某个字典的安全迭代器，和上面有什么区别，具体看函数定义部分
dictIterator *dictGetSafeIterator(dict *d);
// 根据字典迭代器，获取字典中下一个dictEntry
dictEntry *dictNext(dictIterator *iter);
// 释放迭代器
void dictReleaseIterator(dictIterator *iter);
// 返回一个字典的随机的dictEntry
dictEntry *dictGetRandomKey(dictw *d);
//
unsigned int dictGetSomeKeys(dict *d, dictEntry **des, unsigned int count);
//打印字典的状态，分两部分 _dictPrintStatsHt 和 是否正在rehash的判断
void dictPrintStats(dict *d);
//
unsigned int dictGenHashFunction(const void *key, int len);
//
unsigned int dictGenCaseHashFunction(const unsigned char *buf, int len);
//
void dictEmpty(dict *d, void(callback)(void*));
//启用resize
void dictEnableResize(void);
//停止resize
void dictDisableResize(void);
//rehash操作
int dictRehash(dict *d, int n);
//
int dictRehashMilliseconds(dict *d, int ms);
//
void dictSetHashFunctionSeed(unsigned int initval);
//
unsigned int dictGetHashFunctionSeed(void);
// 定义字典扫描
unsigned long dictScan(dict *d, unsigned long v, dictScanFunction *fn, void *privdata);

/* Hash table types */ 
extern dictType dictTypeHeapStringCopyKey;
extern dictType dictTypeHeapStrings;
extern dictType dictTypeHeapStringCopyKeyValue;

#endif /* __DICT_H */
