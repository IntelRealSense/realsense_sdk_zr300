// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// cache line
#define ALIGN 32

void *aligned_malloc(int size)
{
    void *mem = malloc(size+ALIGN+sizeof(void*));
    void **ptr = (void**)((uintptr_t)(mem+ALIGN+sizeof(void*)) & ~(ALIGN-1));
    ptr[-1] = mem;
    return ptr;
}

void aligned_free(void *ptr)
{
    free(((void**)ptr)[-1]);
}
