// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs/core/correlated_sample_set.h"

namespace rs
{
    namespace core
    {
        /**
         * @brief The sample_set_releaser struct
         * a releaser to be used with heap allocated correlated_sample_set
         */
        struct sample_set_releaser
        {
            void operator()(correlated_sample_set * sample_set_ptr) const
            {
                if (sample_set_ptr)
                {
                    for(int i = 0; i < static_cast<uint8_t>(stream_type::max); i++)
                    {
                        if(sample_set_ptr->images[i])
                        {
                            sample_set_ptr->images[i]->release();
                            sample_set_ptr->images[i] = nullptr;
                        }
                    }
                    delete sample_set_ptr;
                }
            }
        };
    }
}
