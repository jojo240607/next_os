#ifndef LIST_H
#define LIST_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "node.h"

#define GET_LIST(obj) ((List *)obj)
// 类声明
typedef struct _List List;
typedef struct _ListFun ListFun;
// 类成员函数结构
struct _ListFun {
    void (*destroy)(List* self);
	void (*add)(List* self, Node *data);

	Node * (*at)(List* self, uint8_t index);

	void (*add_int)(List* self, int data);

	int (*at_int)(List* self, uint8_t index);

};
// 类结构
struct _List {
    const ListFun* fun;
    // TODO: 添加数据成员
    Node *head; //头指针
    Node *tail; //尾指针
    volatile size_t size;
};
typedef struct {
    Node base;
    int data;
} int_data_t;
// 构造函数声明
List* list_create();
void list_init(List* self);

// 析构函数声明
void list_deinit(List* self);

#endif // LIST_H