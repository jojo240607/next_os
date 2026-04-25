#ifndef QUEUE_H
#define QUEUE_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "node.h"

#define GET_QUEUE(obj) ((Queue *)obj)
// 类声明
typedef struct _Queue Queue;
typedef struct _QueueFun QueueFun;
// 类成员函数结构
struct _QueueFun {
    void (*destroy)(Queue* self);
	void (*enqueue)(Queue* self, Node *data);
	Node * (*dequeue)(Queue* self);

};
// 类结构
struct _Queue {
    const QueueFun* fun;
    // TODO: 添加数据成员
    Node *head; //头指针
    Node *tail; //尾指针
    size_t size;
};

// 构造函数声明
Queue* queue_create();
void queue_init(Queue* self);

// 析构函数声明
void queue_deinit(Queue* self);

#endif // QUEUE_H