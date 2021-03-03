#ifndef RB_TREE_RB_TREE_H
#define RB_TREE_RB_TREE_H

#include "stdio.h"

struct RbTree;
struct RbNode;

enum RbErrno {
    RB_ENOMEM = 1,
    RB_EINVAL = 2,
};


int RbConstruct (struct RbTree** rbTree);
int RbDestructor(struct RbTree*  rbTree);

int RbInsert (      struct RbTree* rbTree, int key);
int RbErase  (      struct RbTree* rbTree, int key);
int RbFind   (const struct RbTree* rbTree, int key, struct RbNode** rbNode);
int RbForeach(      struct RbTree* rbTree, 
              int (*function)(struct RbTree* rbTree, 
                              struct RbNode* rbNode, void* data), 
              void* data);

int RbDump(FILE* fileDot, struct RbTree* rbTree);

#endif //RB_TREE_RB_TREE_H
