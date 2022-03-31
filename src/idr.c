
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#include "idr.h"
#include <stdlib.h>

static long idr_rb_cmp(const struct rb_node *rba, const struct rb_node *rbb)
{
    struct idr_node *node_a = rb_to_idr(rba);
    struct idr_node *node_b = rb_to_idr(rbb);
    return node_a->index < node_b->index ? -1 : 1;
}

static long idr_rb_find(const struct rb_node *rb, const void *key)
{
    struct idr_node *node = rb_to_idr(rb);
    if (node->index == (unsigned long)key) return 0;
    return node->index > (unsigned long)key ? -1 : 1;
}

/**
 * idr_get_free_range - get the first unused id
 * @idr: the idr handle
 * @min: minimum id to get
 * @max: maximum id to get
 * @nextp: return the next linked list node
 */
static unsigned long idr_get_free_range(struct idr_root *idr, unsigned long min, unsigned long max, bool *fusep,
                                        struct list_head **nextp, struct rb_node **parentp, struct rb_node ***linkp)
{
    struct idr_node *node;
    struct rb_node *rb;
    unsigned long walk;

    min = max(min, 1);

    *fusep = !!(rb = rb_find_last(&idr->root, (void *)min, idr_rb_find, parentp, linkp));
    if (unlikely(!(node = rb_to_idr_safe(rb)))) {
        if (!*parentp)
            *nextp = idr->list.next;
        else if (*linkp == &(*parentp)->left)
            *nextp = &rb_to_idr(*parentp)->list;
        else if (*linkp == &(*parentp)->right)
            *nextp = rb_to_idr(*parentp)->list.next;
        return min;
    }

    for (walk = min + 1; walk < max; ++walk) {
        if (list_check_end(&idr->list, &node->list))
            goto finish;

        node = list_next_entry(node, list);
        if (node->index != walk)
            goto finish;
    }

    return IDR_NONE;

finish:
    *nextp = node->list.next;
    return walk;
}

/**
 * idr_node_alloc_range - sequence add an node to the idr
 * @idr: the idr handle
 * @node: idr node to add
 * @pdata: node pdata
 * @min: minimum id
 * @max: maximum id
 */
unsigned long idr_node_alloc_range(struct idr_root *idr, struct idr_node *node, void *pdata, unsigned long min, unsigned long max)
{
    struct rb_node *parent, **link;
    struct list_head *next = NULL;
    unsigned long id;
    bool fuse;

    id = idr_get_free_range(idr, min, max, &fuse, &next, &parent, &link);
    if (unlikely(id == IDR_NONE))
        goto finish;

    node->index = id;
    node->pdata = pdata;
    list_add_prev(next, &node->list);

    if (!fuse && parent)
        rb_insert_node(&idr->root, parent, link, &node->rb);
    else
        rb_insert(&idr->root, &node->rb, idr_rb_cmp);

finish:
    return id + idr->id_base;
}

/**
 * idr_node_alloc_cyclic_range - cyclic add an node to the idr
 * @idr: the idr handle
 * @node: idr node to add
 * @pdata: node pdata
 * @min: minimum id
 * @max: maximum id
 */
unsigned long idr_node_alloc_cyclic_range(struct idr_root *idr, struct idr_node *node, void *pdata, unsigned long min, unsigned long max)
{
    unsigned long next;
    unsigned long id;

    next = clamp(idr->id_next, min, max);

    id = idr_node_alloc_range(idr, node, pdata, next, max);
    if (id == IDR_NONE)
        id = idr_node_alloc_range(idr, node, pdata, min, max);

    if (id == IDR_NONE)
        return id;

    idr->id_next = id + 1;
    return id + idr->id_base;
}

/**
 * idr_node_free - remove an id from the idr
 * @idr: the idr handle
 * @id: the id to release
 */
struct idr_node *idr_node_free(struct idr_root *idr, unsigned long id)
{
    struct idr_node *node;
    struct rb_node *rb;

    id -= idr->id_base;

    rb = rb_find(&idr->root, (void *)id, idr_rb_find);
    if (!(node = rb_to_idr_safe(rb)))
        goto finish;

    list_del(&node->list);
    rb_delete(&idr->root, &node->rb);

finish:
    return node;
}

/**
 * idr_alloc_range - alloc a node and sequence add the id to idr
 * @idr: the idr handle
 * @pdata: node pdata
 * @min: minimum id
 * @max: maximum id
 */
unsigned long idr_alloc_range(struct idr_root *idr, void *pdata, unsigned long min, unsigned long max)
{
    struct idr_node *node;
    unsigned long id;

    node = malloc(sizeof(*node));
    if (!node)
        return IDR_NONE;

    id = idr_node_alloc_range(idr, node, pdata, min, max);
    if (unlikely(id == IDR_NONE))
        free(node);

    return id;
}

/**
 * idr_alloc_cyclic_range - alloc a node and cyclic add the id to idr
 * @idr: the idr handle
 * @pdata: node pdata
 * @min: minimum id
 * @max: maximum id
 */
unsigned long idr_alloc_cyclic_range(struct idr_root *idr, void *pdata, unsigned long min, unsigned long max)
{
    struct idr_node *node;
    unsigned long id;

    node = malloc(sizeof(*node));
    if (!node)
        return IDR_NONE;

    id = idr_node_alloc_cyclic_range(idr, node, pdata, min, max);
    if (unlikely(id == IDR_NONE))
        free(node);

    return id;
}

/**
 * idr_free - remove id from the idr and free
 * @idr: the idr handle
 * @id: the id to release
 */
void idr_free(struct idr_root *idr, unsigned long id)
{
    struct idr_node *node;

    node = idr_node_free(idr, id);
    if (node)
        free(node);
}

/**
 * idr_find - remove id from the idr and free
 * @idr: the idr handle
 * @id: the id to find
 */
void *idr_find(struct idr_root *idr, unsigned long id)
{
    struct idr_node *node;
    struct rb_node *rb;

    rb = rb_find(&idr->root, (void *)id, idr_rb_find);
    node = rb_to_idr_safe(rb);

    return node ? node->pdata : NULL;
}

/**
 * idr_release - remove id from the idr and free
 * @idr: the idr handle
 */
void idr_release(struct idr_root *idr)
{
    struct idr_node *node, *next;

    list_for_each_entry_safe(node, next, &idr->list, list)
        free(node);

    idr->root = RB_INIT;
    list_head_init(&idr->list);
}

/**
 * idr_root_init - create an idr
 * @base: id number base
 */
void idr_root_init(struct idr_root *idr, unsigned long base)
{
    idr->id_base = base;
    idr->root = RB_INIT;
    list_head_init(&idr->list);
}

/**
 * idr_create - create an idr
 * @base: id number base
 */
struct idr_root *idr_create(unsigned long base)
{
    struct idr_root *idr;

    idr = malloc(sizeof(*idr));
    if (idr)
        idr_root_init(idr, base);

    return idr;
}

/**
 * idr_delete - delete an idr
 * @idr: the idr handle
 */
void idr_delete(struct idr_root *idr)
{
    idr_release(idr);
    free(idr);
}