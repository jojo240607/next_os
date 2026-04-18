//
// Created by Administrator on 2026/4/18/018.
//

#ifndef SYSTEM_NODE_H
#define SYSTEM_NODE_H

#define GET_NODE(obj) ((Node *)obj)
typedef struct _node Node;

struct _node {
    Node *next;
};
#endif //SYSTEM_NODE_H
