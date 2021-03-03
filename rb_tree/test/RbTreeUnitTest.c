
#include "RbTreeUnitTest.h"

#include "assert.h"
#include "limits.h"
#include "stdlib.h"
#include "time.h"

#include "RbTree.h"

static const int RANDOM_MAX = INT_MAX / 10;
static const int RANDOM_MIN = INT_MIN / 10;
static const int KEY_STEP = 10;

static void TestCalloc();
static void TestEinval();
static void TestInsert();
static void TestDump();

static void FillTree(struct RbTree* rbTree);

// ==== Tester functions =======================================================

static void TestCalloc() {
    struct RbTree* rbTree = NULL;
    
    assert(RbConstruct(&rbTree) == RB_ENOMEM);
    assert(RbConstruct(&rbTree) == RB_ENOMEM);
    
    assert(RbConstruct(&rbTree)     == 0);
    assert(RbInsert(rbTree, rand()) == RB_ENOMEM);
    assert(RbInsert(rbTree, rand()) == 0);

    assert(RbDestructor(rbTree) == 0);
}

static void TestEinval() {
    assert(RbConstruct(NULL)           == RB_EINVAL);
    assert(RbInsert(NULL, rand())      == RB_EINVAL);
    assert(RbFind(NULL, rand(), NULL)  == RB_EINVAL); 
    assert(RbForeach(NULL, NULL, NULL) == RB_EINVAL);
    assert(RbDump(NULL, NULL)          == RB_EINVAL);
    assert(RbDestructor(NULL)          == RB_EINVAL);
}

static void TestInsert() {
    struct RbTree* rbTree = NULL;
    assert(RbConstruct(&rbTree) == 0);

    FillTree(rbTree);

    assert(RbDestructor(NULL) == RB_EINVAL);
}

static void TestDump() {
    struct RbTree* rbTree = NULL;
    assert(RbConstruct(&rbTree) == 0);

    FillTree(rbTree);

    FILE* dot = fopen("dot", "w");
    RbDump(dot, rbTree);
    fclose(dot);

    assert(RbDestructor(NULL) == RB_EINVAL);
}

// ==== Help finctions =========================================================

static void FillTree(struct RbTree* rbTree) {
    assert(rbTree);

    int baseKey = rand() % (RANDOM_MAX + 1 - RANDOM_MIN) + RANDOM_MIN;

    assert(RbInsert(rbTree, baseKey)                == 0);
    assert(RbInsert(rbTree, baseKey + KEY_STEP)     == 0);
    assert(RbInsert(rbTree, baseKey + KEY_STEP * 2) == 0);
    assert(RbInsert(rbTree, baseKey + KEY_STEP * 3) == 0);
    assert(RbInsert(rbTree, baseKey - KEY_STEP * 1) == 0);
    assert(RbInsert(rbTree, baseKey - KEY_STEP * 2) == 0);
    assert(RbInsert(rbTree, baseKey + KEY_STEP * 4) == 0);
    assert(RbInsert(rbTree, baseKey + KEY_STEP * 5) == 0);
    assert(RbInsert(rbTree, baseKey + KEY_STEP * 6) == 0);
    assert(RbInsert(rbTree, baseKey + KEY_STEP * 6) == 0);
}


// =============================================================================

void RbTestAll() {
    srand(time(NULL));
    
    TestCalloc();
    TestEinval();
    TestInsert();
    TestDump();
}
