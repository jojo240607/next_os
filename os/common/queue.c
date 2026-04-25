#include "queue.h"
#include "linear_pool.h"
#include <stdio.h>

static void queue_enqueue(Queue* self, Node *data);
static Node * queue_dequeue(Queue* self);
static bool queue_contain(Queue* self, Node *data);

// 析构函数声明
static void queue_destroy(Queue* self);

// TODO: 初始化数据成员
static const QueueFun queue_fun = {
    .destroy = queue_destroy,
	.enqueue = queue_enqueue,
	.dequeue = queue_dequeue,
};
// 构造函数实现
Queue* queue_create() {
    Queue* obj = (Queue*)os_malloc(sizeof(Queue));
    if (obj) {
        memset(obj, 0, sizeof(Queue));
        queue_init(obj);
    }
    return obj;
}

void queue_init(Queue* self) {
    self->fun = &(queue_fun);
    // TODO: 初始化数据成员
    self->head = NULL;
    self->tail = NULL;
    self->size = 0;
}

void queue_deinit(Queue* self) {
    // TODO: 数据成员申请资源释放
}

// 析构函数实现
static void queue_destroy(Queue* self) {
    if (self != NULL) {
        queue_deinit(self);
        os_free(self);
    }
}

// enqueue method
static void queue_enqueue(Queue* self, Node *data) {
    if (NULL == self) {
        return;
    }
    if(queue_contain(self, data)) {
        return;
    }
    if (NULL == data) {
        return;
    }
    data->next = NULL;
    if (self->size == 0) {
        self->tail = data;
        self->head = data;
    } else {
        self->tail->next = data;
        self->tail = data;
    }
    self->size++;
}
// dequeue method
static Node * queue_dequeue(Queue* self) {
    if (NULL == self) {
        return NULL;
    }
    if (self->size == 0) {
        return NULL;
    }
    Node *queue_data = self->head;
    self->head = queue_data->next;
    queue_data->next = NULL;
    self->size--;
    return queue_data;
}

//检测data是否已存在队列中
static bool queue_contain(Queue* self, Node *data) {
    if (NULL == self) {
        return false;
    }
    Node *current = self->head;
    while (current != NULL) {
        if (current == data) {
            return true;
        }
        current = current->next;
    }
    return false;
}

