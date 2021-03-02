
#include "RbTreeUnitTest.h"

#include "RbTree.h"

void RbTestAll() {
    struct RbTree* rbTree;
    RbConstruct(&rbTree);

    RbInsert(rbTree, 10);
    RbInsert(rbTree, 20);
    RbInsert(rbTree, 30);
    RbInsert(rbTree, 5);
    RbInsert(rbTree, 0);
    RbInsert(rbTree, 40);
    RbInsert(rbTree, 60);
    RbInsert(rbTree, 70);

    FILE* dot = fopen("dot", "w");
    RbDump(dot, rbTree);
    fclose(dot);

    RbDestructor(rbTree);
}