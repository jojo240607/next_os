#ifndef NODE_H
#define NODE_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define GET_NODE(obj) ((Node *)obj)
// 类声明
typedef struct _Node Node;
// 类结构
struct _Node {
    // TODO: 添加数据成员
    Node *next;
};

#endif // NODE_H