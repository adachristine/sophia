// vm_tree.c
//
// AUTHOR NOTE:
//
// a great deal of this code is lifted and adapted straight from wikipedia at
// https://en.wikipedia.org/wiki/Red%E2%80%93black_tree
//
// i reserve no rights under copyright law to this material, but i agree to be
// bound by the terms of the Creative Commons Share-Alike by Attribution license
//
// if any of the following is found to be in violation please contact the author
// at ada.christine.18+sophia _at_ gmail _dot_ com
//
// -Ada Christine Fontaine, 1/1/2022
//
// LICENSE NOTIFICATION
//
// THE WORK (AS DEFINED BELOW) IS PROVIDED UNDER THE TERMS OF THIS CREATIVE
// COMMONS PUBLIC LICENSE ("CCPL" OR "LICENSE"). THE WORK IS PROTECTED BY
// COPYRIGHT AND/OR OTHER APPLICABLE LAW. ANY USE OF THE WORK OTHER THAN AS
// AUTHORIZED UNDER THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.

// BY EXERCISING ANY RIGHTS TO THE WORK PROVIDED HERE, YOU ACCEPT AND AGREE TO
// BE BOUND BY THE TERMS OF THIS LICENSE. TO THE EXTENT THIS LICENSE MAY BE
// CONSIDERED TO BE A CONTRACT, THE LICENSOR GRANTS YOU THE RIGHTS CONTAINED
// HERE IN CONSIDERATION OF YOUR ACCEPTANCE OF SUCH TERMS AND CONDITIONS. 
//
// The full text of the CC-BY-SA 3.0 license can be found here:
// https://en.wikipedia.org/wiki/Wikipedia:Text_of_Creative_Commons_Attribution-ShareAlike_3.0_Unported_License

#include <stddef.h>
#include <stdint.h>

#include "vm_tree.h"

#define assert(expr)

#define NIL NULL
#define left child[LEFT]
#define right child[RIGHT]
// the direction of the child node
#define childDir(vmn) (vmn == (vmn->parent)->right ? RIGHT : LEFT)

static int compare(uintptr_t a1, size_t s1, uintptr_t a2, size_t s2)
{
    // case 1: a1:a1+s1 occupies a region entirely below a2
    // and is therefore less than.
    if ((a1 + s1) <= a2)
    {
        return -1;
    }
    // case 2: a1:a1+s1 occupies a region entirely above (a2+s2)
    // and is therefore greater than.
    else if (a1 >= (a2 + s2))
    {
        return 1;
    }
    // case 3: the above conditions are not satisfied and therefore
    // the regions overlap and are considered equivalent.
    return 0;
}

static int compare_key(struct vm_tree_key const * const k1,
        struct vm_tree_key const * const k2)
{
    return compare(k1->address, k1->size, k2->address, k2->size);
}

static struct vm_tree_node* RotateDirRoot(
        struct vm_tree* T,   // red–black tree
        struct vm_tree_node* P,   // root of subtree (may be the root of T)
        enum vm_tree_direction dir) {   // dir ∈ { LEFT, RIGHT }
    struct vm_tree_node* G = P->parent;
    struct vm_tree_node* S = P->child[1-dir];
    struct vm_tree_node* C;
    assert(S != NIL); // pointer to true node required
    C = S->child[dir];
    P->child[1-dir] = C; if (C != NIL) C->parent = P;
    S->child[  dir] = P; P->parent = S;
    S->parent = G;
    if (G != NULL)
        G->child[ P == G->right ? RIGHT : LEFT ] = S;
    else
        T->root = S;
    return S; // new root of subtree
}

#define RotateDir(N,dir) RotateDirRoot(T,N,dir)
#define RotateLeft(N)    RotateDirRoot(T,N,LEFT)
#define RotateRight(N)   RotateDirRoot(T,N,RIGHT)

void vmt_insert(
        struct vm_tree* T,         // -> red–black tree
        struct vm_tree_node* N,  // -> node to be inserted
        struct vm_tree_node* P,  // -> parent node of N ( may be NULL )
        enum vm_tree_direction dir) // side ( LEFT or RIGHT ) of P where to insert N
{
    struct vm_tree_node* G;  // -> parent node of P
    struct vm_tree_node* U;  // -> uncle of N

    N->color = RED;
    N->left  = NIL;
    N->right = NIL;
    N->parent = P;
    if (P == NULL) {   // There is no parent
        T->root = N;     // N is the new root of the tree T.
        return; // insertion complete
    }
    P->child[dir] = N; // insert N as dir-child of P
    // start of the (do while)-loop:
    do {
        if (P->color == BLACK) {
            // Case_I1 (P black):
            return; // insertion complete
        }
        // From now on P is red.
        if ((G = P->parent) == NULL) 
            goto Case_I4; // P red and root
        // else: P red and G!=NULL.
        dir = childDir(P); // the side of parent G on which node P is located
        U = G->child[1-dir]; // uncle
        if (U == NIL || U->color == BLACK) // considered black
            goto Case_I56; // P red && U black
        // Case_I2 (P+U red):
        P->color = BLACK;
        U->color = BLACK;
        G->color = RED;
        N = G; // new current node
        // iterate 1 black level higher
        //   (= 2 tree levels)
    } while ((P = N->parent) != NULL);
    // end of the (do while)-loop
    // Leaving the (do while)-loop (after having fallen through from Case_I2).
    // Case_I3: N is the root and red.
    return; // insertion complete
Case_I4: // P is the root and red:
    P->color = BLACK;
    return; // insertion complete
Case_I56: // P red && U black:
    if (N == P->child[1-dir])
    { // Case_I5 (P red && U black && N inner grandchild of G):
        RotateDir(P,dir); // P is never the root
        N = P; // new current node
        P = G->child[dir]; // new parent of N
        // fall through to Case_I6
    }
    // Case_I6 (P red && U black && N outer grandchild of G):
    RotateDirRoot(T,G,1-dir); // G may be the root
    P->color = BLACK;
    G->color = RED;
    return; // insertion complete
} // end of RBinsert1

void vmt_delete(
        struct vm_tree* T, // -> red–black tree
        struct vm_tree_node* N)  // -> node to be deleted
{
    struct vm_tree_node* P = N->parent;  // -> parent node of N
    enum vm_tree_direction dir; // side of P on which N is located (∈ { LEFT, RIGHT })
    struct vm_tree_node* S;  // -> sibling of N
    struct vm_tree_node* C;  // -> close   nephew
    struct vm_tree_node* D;  // -> distant nephew

    // P != NULL, since N is not the root.
    dir = childDir(N); // side of parent P on which the node N is located
    // Replace N at its parent P by NIL:
    P->child[dir] = NIL;
    goto Start_D;      // jump into the loop

    // start of the (do while)-loop:
    do {
        dir = childDir(N);   // side of parent P on which node N is located
Start_D:
        S = P->child[1-dir]; // sibling of N (has black height >= 1)
        D = S->child[1-dir]; // distant nephew
        C = S->child[  dir]; // close   nephew
        if (S->color == RED)
            goto Case_D3;                  // S red ===> P+C+D black
        // S is black:
        if (D != NIL && D->color == RED) // not considered black
            goto Case_D6;                  // D red && S black
        if (C != NIL && C->color == RED) // not considered black
            goto Case_D5;                  // C red && S+D black
        // Here both nephews are == NIL (first iteration) or black (later).
        if (P->color == RED)
            goto Case_D4;                  // P red && C+S+D black
        // Case_D1 (P+C+S+D black):
        S->color = RED;
        N = P; // new current node (maybe the root)
        // iterate 1 black level
        //   (= 1 tree level) higher
    } while ((P = N->parent) != NULL);
    // end of the (do while)-loop
    // Case_D2 (P == NULL):
    return; // deletion complete
Case_D3: // S red && P+C+D black:
    RotateDirRoot(T,P,dir); // P may be the root
    P->color = RED;
    S->color = BLACK;
    S = C; // != NIL
    // now: P red && S black
    D = S->child[1-dir]; // distant nephew
    if (D != NIL && D->color == RED)
        goto Case_D6;      // D red && S black
    C = S->child[  dir]; // close   nephew
    if (C != NIL && C->color == RED)
        goto Case_D5;      // C red && S+D black
    // Otherwise C+D considered black.
    // fall through to Case_D4
    // Case_D4: // P red && S+C+D black:
    S->color = RED;
    P->color = BLACK;
    return; // deletion complete
Case_D4: // P red && S+C+D black:
  S->color = RED;
  P->color = BLACK;
  return; // deletion complete
Case_D5: // C red && S+D black:
    RotateDir(S,1-dir); // S is never the root
    S->color = RED;
    C->color = BLACK;
    D = S;
    S = C;
    // now: D red && S black
    // fall through to Case_D6
Case_D6: // D red && S black:
    RotateDirRoot(T,P,dir); // P may be the root
    S->color = P->color;
    P->color = BLACK;
    D->color = BLACK;
    return; // deletion complete
} // end of RBdelete2

enum vm_tree_direction vmn_child_direction(
        struct vm_tree_node *n,
        struct vm_tree_node *p
)
{
    if (p && compare_key(&p->key, &n->key) > 0)
    {
        return LEFT;
    }
    return RIGHT;
}

struct vm_tree_node *vmt_search_key(
        struct vm_tree *tree,
        struct vm_tree_key *key)
{
    struct vm_tree_node *N = tree->root;
    
    while (N && (compare_key(&N->key, key) != 0))
    {
        if (compare_key(&N->key, key) > 0)
        {
            N = N->left;
        }
        else
        {
            N = N->right;
        }
    }

    return N;
}

// search for the predecessor node for a given key
struct vm_tree_node *vmn_predecessor_key(
        struct vm_tree_node *N,
        struct vm_tree_key *key)
{
    struct vm_tree_node *P = NULL;

    while (N && (compare_key(&N->key, key) != 0))
    {
        if (compare_key(&N->key, key) > 0)
        {
            N = N->left;
        }
        else
        {
            P = N;
            N = N->right;
        }
    }

    return P;
}

// search for the successor node for a given key
struct vm_tree_node *vmn_successor_key(
        struct vm_tree_node *N,
        struct vm_tree_key *key)
{
    (void)N;
    (void)key;
    //TODO: write
    return NULL;
}

struct vm_tree_node *vmn_min(struct vm_tree_node *node)
{
    while (node && node->left)
    {
        node = node->left;
    }

    return node;
}

struct vm_tree_node *vmn_max(struct vm_tree_node *node)
{
    while (node && node->right)
    {
        node = node->right;
    }

    return node;
}

void *vmt_get_object(struct vm_tree *tree, void *address)
{
    // find the object for a page.
    struct vm_tree_key key = {(uintptr_t)address, 1};
    struct vm_tree_node *node = vmt_search_key(tree, &key);
    if (node)
    {
        return node->object;
    }
    return NULL;
}

