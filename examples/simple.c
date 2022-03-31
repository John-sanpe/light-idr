/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 Sanpe <sanpeqf@gmail.com>
 */

#include "idr.h"
#include <stdio.h>

#define TEST_LEN 20

int main(void)
{
    struct idr_root *root;
    unsigned long nodes[TEST_LEN];
    unsigned int count;

    root = idr_create(0);

    for (count = 0; count < TEST_LEN; ++count) {
        nodes[count] = idr_alloc_cyclic(root, NULL);
        printf("test%02u id: %02lu\n", count, nodes[count]);
    }

    idr_delete(root);
    return 0;
}
