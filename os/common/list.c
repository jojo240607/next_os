#include "list.h"
#include <stdio.h>
#include "../common/linear_pool.h"

static int list_at_int(List* self, uint8_t index);

static void list_add_int(List* self, int data);

static Node * list_at(List* self, uint8_t index);

static void list_add(List* self, Node *data);

// 析构函数声明
static void list_destroy(List* self);

// TODO: 初始化数据成员
static const ListFun list_fun = {
    .destroy = list_destroy,
	.add = list_add,
	.at = list_at,
	.add_int = list_add_int,
	.at_int = list_at_int,
};
// 构造函数实现
List* list_create() {
    List* obj = (List*)os_malloc(sizeof(List));
    if (obj) {
        memset(obj, 0, sizeof(List));
        list_init(obj);
    }
    return obj;
}

void list_init(List* self) {
    self->fun = &(list_fun);
    // TODO: 初始化数据成员
    self->head = NULL;
    self->tail = NULL;
    self->size = 0;
}

void list_deinit(List* self) {
    // TODO: 数据成员申请资源释放
}

// 析构函数实现
static void list_destroy(List* self) {
    if (self != NULL) {
        list_deinit(self);
        os_free(self);
    }
}

// add method
static void list_add(List* self, Node *data) {
    // TODO: add add method
    if (NULL == data) {
        return;
    }
    data->next = NULL;
    if (self->size == 0) {
        self->head = data;
    } else {
        self->tail->next = data;
        self->tail = data;
    }
    self->size++;
}


// at method
static Node *list_at(List* self, uint8_t index) {
    // TODO: add at method
    Node *current = self->head;
    uint8_t curindex = 0;
    while (current) {
        if (curindex == index) {
            return current;
        }
        current = current->next;
        curindex++;
    }
    return NULL;
}


// add_int method
static void list_add_int(List* self, int data) {
    // TODO: add add_int method
    int_data_t *p = os_malloc(sizeof(int_data_t));
    memset(p, 0, sizeof(int_data_t));
    p->data = data;
    list_add(self, GET_NODE(p));
}


// at_int method
static int list_at_int(List* self, uint8_t index) {
    // TODO: add at_int method
    int_data_t *node = (int_data_t *)list_at(self, index);
    if (node) {
        return node->data;
    }
    return -1;
}

