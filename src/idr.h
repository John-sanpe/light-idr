/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _IDR_H_
#define _IDR_H_

#include <limits.h>
#include "list.h"
#include "rbtree.h"

#define IDR_NONE 0

struct idr_root {
    struct rb_root root;
    struct list_head list;
    unsigned long id_base;
    unsigned long id_next;
};

struct idr_node {
    struct list_head list;
    struct rb_node rb;
    unsigned long index;
    void *pdata;
};

#define rb_to_idr(ptr) \
    rb_entry(ptr, struct idr_node, rb)

#define rb_to_idr_safe(ptr) \
    rb_entry_safe(ptr, struct idr_node, rb)

#define list_to_idr(ptr) \
    list_entry(ptr, struct idr_node, list)

/**
 * min - return minimum of two values of the same or compatible types
 * @a: first value
 * @b: second value
 */
#define min(a, b) ({                                    \
    typeof(a) _amin = (a);                              \
    typeof(a) _bmin = (b);                              \
    (void)(&_amin == &_bmin);                           \
    _amin < _bmin ? _amin : _bmin;                      \
})

/**
 * max - return maximum of two values of the same or compatible types
 * @a: first value
 * @b: second value
 */
#define max(a, b) ({                                    \
    typeof(a) _amax = (a);                              \
    typeof(a) _bmax = (b);                              \
    (void)(&_amax == &_bmax);                           \
    _amax > _bmax ? _amax : _bmax;                      \
})

/**
 * clamp - return a value clamped to a given range with strict typechecking.
 * @val: current value.
 * @lo: lowest allowable value.
 * @hi: highest allowable value.
 */
#define clamp(val, lo, hi) (                            \
    min(max(val, lo), hi)                               \
)

extern unsigned long idr_node_alloc_range(struct idr_root *idr, struct idr_node *node, void *pdata, unsigned long min, unsigned long max);
extern unsigned long idr_node_alloc_cyclic_range(struct idr_root *idr, struct idr_node *node, void *pdata, unsigned long min, unsigned long max);
extern struct idr_node *idr_node_free(struct idr_root *idr, unsigned long id);
extern unsigned long idr_alloc_range(struct idr_root *idr, void *pdata, unsigned long min, unsigned long max);
extern unsigned long idr_alloc_cyclic_range(struct idr_root *idr, void *pdata, unsigned long min, unsigned long max);
extern void idr_free(struct idr_root *idr, unsigned long id);
extern void *idr_find(struct idr_root *idr, unsigned long id);
extern void idr_root_init(struct idr_root *idr, unsigned long base);
extern struct idr_root *idr_create(unsigned long base);
extern void idr_release(struct idr_root *idr);
extern void idr_delete(struct idr_root *idr);

#define IDR_ALLOC_OP(name, function)                                                                        \
static inline unsigned long                                                                                 \
generic_idr_node_##name(struct idr_root *idr, struct idr_node *node, void *pdata)                           \
{                                                                                                           \
    return idr_node_##function(idr, node, pdata, 0, ULONG_MAX);                                             \
}                                                                                                           \
                                                                                                            \
static inline unsigned long                                                                                 \
generic_idr_node_##name##_min(struct idr_root *idr, struct idr_node *node, void *pdata, unsigned long min)  \
{                                                                                                           \
    return idr_node_##function(idr, node, pdata, min, ULONG_MAX);                                           \
}                                                                                                           \
                                                                                                            \
static inline unsigned long                                                                                 \
generic_idr_node_##name##_max(struct idr_root *idr, struct idr_node *node, void *pdata, unsigned long max)  \
{                                                                                                           \
    return idr_node_##function(idr, node, pdata, 0, max);                                                   \
}                                                                                                           \
                                                                                                            \
static inline unsigned long                                                                                 \
generic_idr_##name(struct idr_root *idr, void *pdata)                                                       \
{                                                                                                           \
    return idr_##function(idr, pdata, 0, ULONG_MAX);                                                        \
}                                                                                                           \
                                                                                                            \
static inline unsigned long                                                                                 \
generic_idr_##name##_min(struct idr_root *idr, void *pdata, unsigned long min)                              \
{                                                                                                           \
    return idr_##function(idr, pdata, min, ULONG_MAX);                                                      \
}                                                                                                           \
                                                                                                            \
static inline unsigned long                                                                                 \
generic_idr_##name##_max(struct idr_root *idr, void *pdata, unsigned long max)                              \
{                                                                                                           \
    return idr_##function(idr, pdata, 0, max);                                                              \
}

IDR_ALLOC_OP(alloc, alloc_range)
#define idr_node_alloc(idr, node, pdata)                    generic_idr_node_alloc(idr, node, pdata)
#define idr_node_alloc_min(idr, node, pdata, min)           generic_idr_node_alloc_min(idr, node, pdata, min)
#define idr_node_alloc_max(idr, node, pdata, max)           generic_idr_node_alloc_max(idr, node, pdata, max)
#define idr_alloc(idr, pdata)                               generic_idr_alloc(idr, pdata)
#define idr_alloc_min(idr, pdata, min)                      generic_idr_alloc_min(idr, pdata, min)
#define idr_alloc_max(idr, pdata, max)                      generic_idr_alloc_max(idr, pdata, max)

IDR_ALLOC_OP(alloc_cyclic, alloc_cyclic_range)
#define idr_node_alloc_cyclic(idr, node, pdata)             generic_idr_node_alloc_cyclic(idr, node, pdata)
#define idr_node_alloc_cyclic_min(idr, node, pdata, min)    generic_idr_node_alloc_cyclic_min(idr, node, pdata, min)
#define idr_node_alloc_cyclic_max(idr, node, pdata, max)    generic_idr_node_alloc_cyclic_max(idr, node, pdata, max)
#define idr_alloc_cyclic(idr, pdata)                        generic_idr_alloc_cyclic(idr, pdata)
#define idr_alloc_cyclic_min(idr, pdata, min)               generic_idr_alloc_cyclic_min(idr, pdata, min)
#define idr_alloc_cyclic_max(idr, pdata, max)               generic_idr_alloc_cyclic_max(idr, pdata, max)

#endif  /* _IDR_H_ */
