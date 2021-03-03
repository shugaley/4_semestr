
#include "RbTree.h"

#include "assert.h"
#include "stdlib.h"

struct RbNode {
    struct RbNode* parent;
    struct RbNode* left;
    struct RbNode* right;

    int key;
    int color;
};

struct RbTree {
    struct RbNode* root;
    struct RbNode* nil;
};

enum Color {
    RED = 1,
    BLACK = 2,
};

#ifndef TEST
#define CALLOC(n, size) calloc(n, size)
#else
#define CALLOC(n, size) TestCalloc(n, size)
#endif

static struct RbNode* CreateRbNode(int color, int key, struct RbNode* nil);
static void ClearNode(struct RbNode* n, const struct RbNode* nil);
static struct RbNode* FindRoot(const struct RbNode* n);
static void ForeachNode(struct RbTree* rbTree, 
                        int (*function)(struct RbTree* rbTree, 
                                        struct RbNode* rbNode, void* data), 
                        void* data, struct RbNode* n);
static void DumpNode(FILE* fileDot, const struct RbNode* n, 
                                    const struct RbNode* nil);

static struct RbNode* GetGrandparent(const struct RbNode* n);
static struct RbNode* GetUncle      (const struct RbNode* n);

static void InsertNewNode(struct RbNode* nNew, 
                          struct RbNode* n, const struct RbNode* nil);
static void InsertCase1(      struct RbNode* n);
static void InsertCase2(const struct RbNode* n);
static void InsertCase3(const struct RbNode* n);
static void InsertCase4(const struct RbNode* n);
static void InsertCase5(const struct RbNode* n);

static void RotateLeft(struct RbNode* n);
static void RotateRight(struct RbNode* n);

static void* TestCalloc(size_t n, size_t size);


// ==== Other helper functions =================================================

static struct RbNode* CreateRbNode(int color, int key, struct RbNode* nil) {
    assert(color == BLACK || color == RED);

    struct RbNode* n = (struct RbNode*)CALLOC(1, sizeof(*n));
    if (n == NULL)
        return NULL; 

    n->parent = NULL;
    n->left   = nil;
    n->right  = nil;

    n->color = color;
    n->key   = key;

    return n;
}

static void ClearNode(struct RbNode* n, const struct RbNode* nil) {

    assert(n);
    assert(nil);

    if (n->left != nil)
        ClearNode(n->left, nil);

    if (n->right != nil)
        ClearNode(n->right, nil);

    n->color  = 0;
    n->key    = 0;
    n->left   = NULL;
    n->right  = NULL;
    n->parent = NULL;

    free(n);
}

static struct RbNode* FindRoot(const struct RbNode* n) {
    assert(n);

    const struct RbNode* root = n;
    while (root->parent)
        root = root->parent;

    return (struct RbNode*)root;
}

static void ForeachNode(struct RbTree* rbTree, 
                        int (*function)(struct RbTree* rbTree, 
                                        struct RbNode* rbNode, void* data), 
                        void* data, struct RbNode* n) {
    assert(rbTree);
    assert(function);
    assert(data);
    assert(n);

    if (n->left != rbTree->nil)
        ForeachNode(rbTree, function, data, n->left);
    if (n->right != rbTree->nil)
        ForeachNode(rbTree, function, data, n->right);
}

static void DumpNode(FILE* fileDot, const struct RbNode* n, 
                                    const struct RbNode* nil) {
    assert(n);
    assert(fileDot);

    fprintf(fileDot, "\n\t\tkey_%d [label = \"%d\", ", n->key, n->key);
    if (n->color == RED) 
        fprintf(fileDot, "color = \"#FF0000\", fontcolor = \"#FFFFFF\" "
                         "style = \"filled\", fillcolor = \"#FF0000\"]\n");
    else 
        fprintf(fileDot, "color = \"#000000\", fontcolor = \"#FFFFFF\" "
                         "style = \"filled\", fillcolor = \"#000000\"]\n");

    if (n->left != nil) {
        fprintf(fileDot, "\t\tkey_%d -> key_%d;\n", n->key, n->left->key);
        DumpNode(fileDot, n->left, nil);
        fprintf(fileDot, "\t\t{\n\t\t\trank = same;\n"
                         "\t\t\tkey_%d -> ", n->left->key);
    } 
    else { 
        fprintf(fileDot, 
            "\n\t\tnil_left_key_%d [label = \"nil\", shape = \"diamond\", "
            "color = \"#FFFFFF\", fontcolor = \"#000000\"];\n", n->key);
        fprintf(fileDot, "\t\tkey_%d -> nil_left_key_%d;\n", n->key, n->key);
        fprintf(fileDot, "\t\t{\n\t\t\trank = same;\n"
                         "\t\t\tnil_left_key_%d -> ", n->key);
    }

    if (n->right != nil) {
        fprintf(fileDot, "key_%d [color=invis];\n\t\t}\n",   n->right->key);
        fprintf(fileDot, "\t\tkey_%d -> key_%d;\n", n->key, n->right->key);
        DumpNode(fileDot, n->right, nil);
    }
    else {
        fprintf(fileDot, "nil_right_key_%d [color=invis];\n\t\t}\n", n->key);
        fprintf(fileDot, 
            "\n\t\tnil_right_key_%d [label = \"nil\", shape = \"diamond\", "
            "color = \"#FFFFFF\", fontcolor = \"#000000\"];\n", n->key);
        fprintf(fileDot, "\t\tkey_%d -> nil_right_key_%d;\n", n->key, n->key);
    } 
}


// ==== Getter relatives functions =============================================

static struct RbNode* GetGrandparent(const struct RbNode* n) {

    assert(n);

    if (n->parent != NULL)
        return n->parent->parent;
    else 
        return NULL;
}

static struct RbNode* GetUncle(const struct RbNode* n) {

    assert(n);

    struct RbNode* gp = GetGrandparent(n);
    if (gp == NULL)
        return NULL;
    if (n->parent == gp->left)
        return gp->right;
    else 
        return gp->left;
}


// ==== Insert functions =======================================================

static void InsertNewNode(struct RbNode* nNew, 
                          struct RbNode* n, const struct RbNode* nil) {
    assert(n);
    assert(nNew);
    assert(nil);

    if (n->key > nNew->key) {
        if (n->left == nil) {
            nNew->parent = n;
            n->left = nNew;
        } 
        else 
            return InsertNewNode(nNew, n->left, nil);
    } 
    else {
        if (n->right == nil) {
            nNew->parent = n;
            n->right = nNew;
        } 
        else
            return InsertNewNode(nNew, n->right, nil);
    } 
}

static void InsertCase1(struct RbNode* n) {
    assert(n);

    if (n->parent == NULL)
        n->color = BLACK;
    else
        InsertCase2(n);
}

static void InsertCase2(const struct RbNode* n) {
    assert(n);

    if (n->parent->color == BLACK)
        return; // Tree is still valid 
    else
        InsertCase3(n);
}

static void InsertCase3(const struct RbNode* n) {
    assert(n);

    struct RbNode *u = GetUncle(n);

    if ((u != NULL) && (u->color == RED)) {
        n->parent->color = BLACK;
        u->color = BLACK;

        struct RbNode* gp = GetGrandparent(n);
        gp->color = RED;

        InsertCase1(gp);
    } 
    else 
        InsertCase4(n);
}

static void InsertCase4(const struct RbNode* n) {
    assert(n);

    struct RbNode* gp = GetGrandparent(n);

    if ((n == n->parent->right) && (n->parent == gp->left)) {
        RotateLeft(n->parent);
        n = n->left;
    } 
    else if ((n == n->parent->left) && (n->parent == gp->right)) {
        RotateRight(n->parent);
        n = n->right;
    }

    InsertCase5(n);
}

static void InsertCase5(const struct RbNode* n) {
    assert(n);

    struct RbNode* gp = GetGrandparent(n);

    n->parent->color = BLACK;
    gp->color = RED;

    if ((n == n->parent->left) && (n->parent == gp->left))
        RotateRight(gp);
    else 
        RotateLeft(gp);
}


// ==== Rotate functions =======================================================

static void RotateLeft(struct RbNode* n) {
    assert(n);

    struct RbNode* pivot = n->right;
	
    pivot->parent = n->parent; // maybe pivot become to root
    if (n->parent != NULL) {
        if (n->parent->left == n)
            n->parent->left = pivot;
        else
            n->parent->right = pivot;
    }		
	
    n->right = pivot->left;
    if (pivot->left != NULL)
        pivot->left->parent = n;

    n->parent = pivot;
    pivot->left = n;
}

static void RotateRight(struct RbNode* n) {
    assert(n);

    struct RbNode* pivot = n->left;
	
    pivot->parent = n->parent; // maybe pivot become to root
    if (n->parent != NULL) {
        if (n->parent->left == n)
            n->parent->left = pivot;
        else
            n->parent->right = pivot;
    }		
	
    n->left = pivot->right;
    if (pivot->right != NULL)
        pivot->right->parent = n;

    n->parent = pivot;
    pivot->right = n;
}


// ==== Tester functions =======================================================

static void* TestCalloc(size_t n, size_t size) {
    static size_t num = 0;

    if(num == 0 || num == 2 || num == 5) {
        num++;
        return NULL;
    }

    num++;
    return calloc(n, size);
}


// =============================================================================
// ==== API ====================================================================

int RbConstruct(struct RbTree** rbTree) {
    if(rbTree == NULL)
        return RB_EINVAL;

    *rbTree = (struct RbTree*)CALLOC(1, sizeof(**rbTree));
    if (*rbTree == NULL) 
        return RB_ENOMEM;
     
    (*rbTree)->nil = CreateRbNode(BLACK, 0, NULL);
    if ((*rbTree)->nil == NULL)
        return RB_ENOMEM;

    (*rbTree)->root = (*rbTree)->nil;
    
    return 0;
}

int RbDestructor(struct RbTree* rbTree) {
    if(rbTree == NULL)
        return RB_EINVAL;

    ClearNode(rbTree->root, rbTree->nil);
    rbTree->root = NULL;

    free(rbTree->nil);
    rbTree->nil = NULL;

    free(rbTree);

    return 0;
}

int RbInsert (struct RbTree* rbTree, int key) {
    if (rbTree == NULL)
        return RB_EINVAL;
    
    struct RbNode* rbNodeSearch = NULL;
    int res = RbFind(rbTree, key, &rbNodeSearch);
    if (res != 0)
        return res;
    if (rbNodeSearch != NULL)
        return 0;

    struct RbNode* rbNodeNew = CreateRbNode(RED, key, rbTree->nil);
    if (rbNodeNew == NULL)
        return RB_ENOMEM;

    if (rbTree->root != rbTree->nil)
        InsertNewNode(rbNodeNew, rbTree->root, rbTree->nil);

    InsertCase1(rbNodeNew);

    rbTree->root = FindRoot(rbNodeNew);

    return 0;
}

int RbErase(struct RbTree* rbTree, int key);

int RbFind(const struct RbTree* rbTree, int key, struct RbNode** rbNode) {
    if (rbTree == NULL || rbNode == NULL)
        return RB_EINVAL;

    *rbNode = rbTree->root;

    while (*rbNode != rbTree->nil) {
        if ((*rbNode)->key > key)
            *rbNode = (*rbNode)->left;
        else if ((*rbNode)->key < key)
            *rbNode = (*rbNode)->right;
        else 
            return 0;
    }

    *rbNode = NULL;
    return 0;
}

int RbForeach(struct RbTree* rbTree, 
              int (*function)(struct RbTree* rbTree, 
                              struct RbNode* rbNode, void* data), 
              void* data) {

    if(rbTree == NULL || function == NULL || data == NULL)
        return RB_EINVAL;             

    ForeachNode(rbTree, function, data, rbTree->root);
    return 0;
}

int RbGetKey (const struct RbNode* rbNode, int* key) {
    if (rbNode == NULL || key == NULL)
        return RB_EINVAL;

    *key = rbNode->key;
    return 0;
}

int RbDump(FILE* fileDot, struct RbTree* rbTree) {
    if (rbTree == NULL || fileDot == NULL) 
        return RB_EINVAL;

    fprintf(fileDot, "digraph RbTree {\n");
    fprintf(fileDot, 
        "\tnode [shape = \"ellipse\", color = \"#000000\", fontsize = 12];\n"
        "\tedge [color = \"#000000\", fontsize = 12];\n\n");

    DumpNode(fileDot, rbTree->root, rbTree->nil);

    fprintf(fileDot, "}\n");

    return 0;
}

