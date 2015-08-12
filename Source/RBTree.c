/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#include "RBTree.h"

#include <stddef.h>
#include <assert.h>


static void RBTree_FixNodeInsertion(struct RBTree *, struct RBTreeNode *);
static void RBTree_FixNodeRemoval(struct RBTree *, struct RBTreeNode *);
static void RBTree_RotateNodeLeft(struct RBTree *, struct RBTreeNode *);
static void RBTree_RotateNodeRight(struct RBTree *, struct RBTreeNode *);


void
RBTree_Initialize(struct RBTree *self)
{
    assert(self != NULL);
    self->nil.color = RBTreeNodeBlack;
    self->root = &self->nil;
}


void
RBTree_InsertNode(struct RBTree *self, struct RBTreeNode *node
                  , int (*nodeComparer)(const struct RBTreeNode *, const struct RBTreeNode *))
{
    assert(self != NULL);
    assert(node != NULL);
    assert(nodeComparer != NULL);
    struct RBTreeNode *nodeParent = &self->nil;
    struct RBTreeNode **nodeParentChild = &self->root;

    while (*nodeParentChild != &self->nil) {
        nodeParent = *nodeParentChild;

        if (nodeComparer(node, nodeParent) < 0) {
            nodeParentChild = &nodeParent->leftChild;
        } else {
            nodeParentChild = &nodeParent->rightChild;
        }
    }

    *nodeParentChild = node;
    node->parent = nodeParent;
    node->leftChild = &self->nil;
    node->rightChild = &self->nil;
    node->color = RBTreeNodeRed;
    RBTree_FixNodeInsertion(self, node);
}


void
RBTree_RemoveNode(struct RBTree *self, const struct RBTreeNode *node)
{
    assert(self != NULL);
    assert(node != NULL);
    struct RBTreeNode *node1;
    struct RBTreeNode *node1Child;

    if (node->leftChild == &self->nil) {
        node1 = (struct RBTreeNode *)node;
        node1Child = node->rightChild;
    } else if (node->rightChild == &self->nil) {
        node1 = (struct RBTreeNode *)node;
        node1Child = node->leftChild;
    } else {
        struct RBTreeNode *nodePrev;
        struct RBTreeNode *nodeNext;

        for (nodePrev = node->leftChild, nodeNext = node->rightChild
             ;; nodePrev = nodePrev->rightChild, nodeNext = nodeNext->leftChild) {
            if (nodePrev->rightChild == &self->nil) {
                node1 = nodePrev;
                node1Child = nodePrev->leftChild;
                break;
            }

            if (nodeNext->leftChild == &self->nil) {
                node1 = nodeNext;
                node1Child = nodeNext->rightChild;
                break;
            }
        }
    }

    if (node1->parent == &self->nil) {
        self->root = node1Child;
    } else {
        if (node1 == node1->parent->leftChild) {
            node1->parent->leftChild = node1Child;
        } else {
            node1->parent->rightChild = node1Child;
        }
    }

    node1Child->parent = node1->parent;
    int isBroken = node1->color == RBTreeNodeBlack;

    if (node1 != node) {
        if (node->parent == &self->nil) {
            self->root = node1;
        } else {
            if (node == node->parent->leftChild) {
                node->parent->leftChild = node1;
            } else {
                node->parent->rightChild = node1;
            }
        }

        node1->parent = node->parent;
        (node1->leftChild = node->leftChild)->parent = node1;
        (node1->rightChild = node->rightChild)->parent = node1;
        node1->color = node->color;
    }

    if (isBroken) {
        RBTree_FixNodeRemoval(self, node1Child);
    }
}


struct RBTreeNode *
RBTree_Search(const struct RBTree *self, uintptr_t key, int (*nodeMatcher)(const struct RBTreeNode *
                                                                           , uintptr_t))
{
    assert(self != NULL);
    assert(nodeMatcher != NULL);
    struct RBTreeNode *node = self->root;

    while (node != &self->nil) {
        int delta = nodeMatcher(node, key);

        if (delta == 0) {
            return node;
        }

        if (delta < 0) {
            node = node->rightChild;
        } else {
            node = node->leftChild;
        }
    }

    return NULL;
}


struct RBTreeNode *
RBTree_FindMin(const struct RBTree *self)
{
    assert(self != NULL);
    struct RBTreeNode *min = self->root;

    if (min == &self->nil) {
        return NULL;
    }

    while (min->leftChild != &self->nil) {
        min = min->leftChild;
    }

    return min;
}


struct RBTreeNode *
RBTree_FindMax(const struct RBTree *self)
{
    assert(self != NULL);
    struct RBTreeNode *max = self->root;

    if (max == &self->nil) {
        return NULL;
    }

    while (max->rightChild != &self->nil) {
        max = max->rightChild;
    }

    return max;
}


static void
RBTree_FixNodeInsertion(struct RBTree *self, struct RBTreeNode *node)
{
    struct RBTreeNode *nodeParent = node->parent;

    while (nodeParent->color == RBTreeNodeRed) {
        struct RBTreeNode *nodeGrandparent = nodeParent->parent;

        if (nodeParent == nodeGrandparent->leftChild) {
            struct RBTreeNode *nodeAuncle = nodeGrandparent->rightChild;

            if (nodeAuncle->color == RBTreeNodeRed) {
                nodeParent->color = RBTreeNodeBlack;
                nodeGrandparent->color = RBTreeNodeRed;
                nodeAuncle->color = RBTreeNodeBlack;
                node = nodeGrandparent;
                nodeParent = node->parent;
                continue;
            }

            if (node == nodeParent->rightChild) {
                RBTree_RotateNodeLeft(self, nodeParent);
                struct RBTreeNode *temp = node;
                node = nodeParent;
                nodeParent = temp;
            }

            nodeParent->color = RBTreeNodeBlack;
            nodeGrandparent->color = RBTreeNodeRed;
            RBTree_RotateNodeRight(self, nodeGrandparent);
        } else {
            struct RBTreeNode *nodeAuncle = nodeGrandparent->leftChild;

            if (nodeAuncle->color == RBTreeNodeRed) {
                nodeParent->color = RBTreeNodeBlack;
                nodeGrandparent->color = RBTreeNodeRed;
                nodeAuncle->color = RBTreeNodeBlack;
                node = nodeGrandparent;
                nodeParent = node->parent;
                continue;
            }

            if (node == nodeParent->leftChild) {
                RBTree_RotateNodeRight(self, nodeParent);
                struct RBTreeNode *temp = node;
                node = nodeParent;
                nodeParent = temp;
            }

            nodeParent->color = RBTreeNodeBlack;
            nodeGrandparent->color = RBTreeNodeRed;
            RBTree_RotateNodeLeft(self, nodeGrandparent);
        }
    }

    self->root->color = RBTreeNodeBlack;
}


static void
RBTree_FixNodeRemoval(struct RBTree *self, struct RBTreeNode *node)
{
    while (node->color == RBTreeNodeBlack && node != self->root) {
        struct RBTreeNode *nodeParent = node->parent;

        if (node == nodeParent->leftChild) {
            struct RBTreeNode *nodeSibling = nodeParent->rightChild;

            if (nodeSibling->color == RBTreeNodeRed) {
                nodeParent->color = RBTreeNodeRed;
                nodeSibling->color = RBTreeNodeBlack;
                RBTree_RotateNodeLeft(self, nodeParent);
                nodeSibling = nodeParent->rightChild;
            }

            struct RBTreeNode *nodeNibling1 = nodeSibling->rightChild;
            struct RBTreeNode *nodeNibling2 = nodeSibling->leftChild;

            if (nodeNibling1->color == RBTreeNodeBlack && nodeNibling2->color == RBTreeNodeBlack) {
                nodeSibling->color = RBTreeNodeRed;
                node = nodeParent;
                continue;
            }

            if (nodeNibling1->color == RBTreeNodeBlack) {
                nodeSibling->color = RBTreeNodeRed;
                nodeNibling2->color = RBTreeNodeBlack;
                RBTree_RotateNodeRight(self, nodeSibling);
                nodeNibling1 = nodeSibling;
                nodeSibling = nodeNibling2;
            }

            nodeSibling->color = nodeParent->color;
            nodeParent->color = RBTreeNodeBlack;
            nodeNibling1->color = RBTreeNodeBlack;
            RBTree_RotateNodeLeft(self, nodeParent);
            node = self->root;
        } else {
            struct RBTreeNode *nodeSibling = nodeParent->leftChild;

            if (nodeSibling->color == RBTreeNodeRed) {
                nodeParent->color = RBTreeNodeRed;
                nodeSibling->color = RBTreeNodeBlack;
                RBTree_RotateNodeRight(self, nodeParent);
                nodeSibling = nodeParent->leftChild;
            }

            struct RBTreeNode *nodeNibling1 = nodeSibling->leftChild;
            struct RBTreeNode *nodeNibling2 = nodeSibling->rightChild;

            if (nodeNibling1->color == RBTreeNodeBlack && nodeNibling2->color == RBTreeNodeBlack) {
                nodeSibling->color = RBTreeNodeRed;
                node = nodeParent;
                continue;
            }

            if (nodeNibling1->color == RBTreeNodeBlack) {
                nodeSibling->color = RBTreeNodeRed;
                nodeNibling2->color = RBTreeNodeBlack;
                RBTree_RotateNodeLeft(self, nodeSibling);
                nodeNibling1 = nodeSibling;
                nodeSibling = nodeNibling2;
            }

            nodeSibling->color = nodeParent->color;
            nodeParent->color = RBTreeNodeBlack;
            nodeNibling1->color = RBTreeNodeBlack;
            RBTree_RotateNodeRight(self, nodeParent);
            node = self->root;
        }
    }

    node->color = RBTreeNodeBlack;
}


static void
RBTree_RotateNodeLeft(struct RBTree *self, struct RBTreeNode *node)
{
    struct RBTreeNode *nodeChild = node->rightChild;
    node->rightChild = nodeChild->leftChild;

    if (nodeChild->leftChild != &self->nil) {
        nodeChild->leftChild->parent = node;
    }

    if (node->parent == &self->nil) {
        self->root = nodeChild;
    } else {
        if (node == node->parent->leftChild) {
            node->parent->leftChild = nodeChild;
        } else {
            node->parent->rightChild = nodeChild;
        }
    }

    nodeChild->parent = node->parent;
    (nodeChild->leftChild = node)->parent = nodeChild;
}


static void
RBTree_RotateNodeRight(struct RBTree *self, struct RBTreeNode *node)
{
    struct RBTreeNode *nodeChild = node->leftChild;
    node->leftChild = nodeChild->rightChild;

    if (nodeChild->rightChild != &self->nil) {
        nodeChild->rightChild->parent = node;
    }

    if (node->parent == &self->nil) {
        self->root = nodeChild;
    } else {
        if (node == node->parent->leftChild) {
            node->parent->leftChild = nodeChild;
        } else {
            node->parent->rightChild = nodeChild;
        }
    }

    nodeChild->parent = node->parent;
    (nodeChild->rightChild = node)->parent = nodeChild;
}
