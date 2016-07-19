// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "image_base.h"

namespace rs
{
    namespace core
    {
        /**
         * @brief The custom_image class
         * implements the sdk image interface for a customized image, where the user provides an allocated image data and
         * an optional image deallocation method with the data_releaser_interface, if no deallocation method is provided,
         * it assumes that the user is handling memory deallocation outside of the custom image class.
         * see complete image documantation in the interface declaration.
         */
        class custom_image : public image_base
        {
        public:
            /**
             * @brief The data_releaser_interface class
             * optional custom deallocation method to be called by the image destructor.
             */
            class data_releaser_interface
            {
            public:
                virtual void release() = 0;
                virtual ~data_releaser_interface() {}
            };

            custom_image(const custom_image &) = delete;
            custom_image & operator = (const custom_image &) = delete;

            custom_image(image_info * info,
                         const void * data,
                         stream_type stream,
                         image_interface::flag flags,
                         uint64_t time_stamp,
                         uint64_t frame_number,
                         rs::utils::smart_ptr<metadata_interface> metadata,
                         rs::utils::smart_ptr<data_releaser_interface> data_releaser);
            image_info query_info(void) const override;
            uint64_t query_time_stamp(void) const override;
            flag query_flags(void) const override;
            const void * query_data(void) const override;
            stream_type query_stream_type() const override;
             uint64_t query_frame_number() const override;
            virtual ~custom_image();
        protected:
            image_info m_info;
            const void * m_data;
            uint64_t m_time_stamp;
            image_interface::flag m_flags;
            stream_type m_stream;
            uint64_t m_frame_number;
            rs::utils::smart_ptr<data_releaser_interface> m_data_releaser;
        };
    }
}
