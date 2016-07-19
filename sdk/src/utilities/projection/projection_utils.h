// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <memory>

#include "rs/core/status.h"

namespace rs
{
    namespace core
    {
        namespace projection_utils
        {
            #define x64_ALIGNMENT(x) (((x)+0x3f)&0xffffffc0)

            static void *aligned_malloc(int size)
            {
                const int align = 32;
                uint8_t *mem = (uint8_t*)malloc(size + align + sizeof(void*));
                void **ptr = (void**)((uintptr_t)(mem + align + sizeof(void*)) & ~(align - 1));
                ptr[-1] = mem;
                return ptr;
            }

            static void aligned_free(void *ptr)
            {
                free(((void**)ptr)[-1]);
            }

            const float MINABS_32F = 1.175494351e-38f;
            const double EPS52 = 2.2204460492503131e-016;
        }
    }
}
