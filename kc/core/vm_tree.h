#pragma once

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

enum vm_tree_direction
{
    LEFT,
    RIGHT
};

enum vm_tree_color
{
    RED,
    BLACK
};


struct vm_tree_key
{
    uintptr_t address;
    size_t size;
};

struct vm_tree_node
{
    // red-black binary tree data
    struct vm_tree_node *parent;
    struct vm_tree_node *child[2];
    enum vm_tree_color color;
    struct vm_tree_key key;
    // object that owns this node
    void *object;
};

struct vm_tree
{
    struct vm_tree_node *root;
};

void vmt_insert(
        struct vm_tree *,
        struct vm_tree_node *,
        struct vm_tree_node *,
        enum vm_tree_direction);

enum vm_tree_direction vmn_child_direction(
        struct vm_tree_node *n,
        struct vm_tree_node *p);

void vmt_delete(struct vm_tree*, struct vm_tree_node*);

struct vm_tree_node *vmt_search_key(struct vm_tree *, struct vm_tree_key *);

struct vm_tree_node *vmn_predecessor_key(
        struct vm_tree_node *,
        struct vm_tree_key *);
struct vm_tree_node *vmn_successor_key(
        struct vm_tree_node *,
        struct vm_tree_key *);

struct vm_tree_node *vmn_min(struct vm_tree_node *node);
struct vm_tree_node *vmn_max(struct vm_tree_node *node);

void *vmt_get_object(struct vm_tree *, void *address);

