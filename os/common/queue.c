/*
 * Queue vs List 区别：
 * - Queue: FIFO 队列，入队检查重复（queue_contain），支持 dequeue_node 按指针移除
 * - List:  单向链表，仅尾部追加，不检查重复，支持按索引访问
 * 调度器就绪队列/等待队列使用 Queue，普通链表场景使用 List。
 */
#include "queue.h"
#include "linear_pool.h"
#include <stdio.h>

static Node * queue_dequeue_node(Queue* self, Node *node);

static void queue_enqueue(Queue* self, Node *data);
static Node * queue_dequeue(Queue* self);
static inline bool queue_contain(Queue* self, Node *data);

// 析构函数声明
static void queue_destroy(Queue* self);

// TODO: 初始化数据成员
static const QueueFun queue_fun = {
    .destroy = queue_destroy,
	.enqueue = queue_enqueue,
	.dequeue = queue_dequeue,
	.dequeue_node = queue_dequeue_node,
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
static inline bool queue_contain(Queue* self, Node *data) {
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


// dequeue_node method
static Node * queue_dequeue_node(Queue* self, Node *node) {
    if (NULL == self) {
        return NULL;
    }
    if (self->size == 0) {
        return NULL;
    }
    Node *priv = NULL;
    Node *current = self->head;
    while (current != NULL) {
        if (current == node) {
            if (priv == NULL) {
                self->head = current->next;
            } else {
                priv->next = current->next;
            }
            // 更新 tail 指针：如果删除的是尾节点
            if (current == self->tail) {
                self->tail = priv;
            }
            current->next = NULL;
            self->size--;
            break;
        }
        priv = current;
        current = current->next;
    }

    return current;
}

