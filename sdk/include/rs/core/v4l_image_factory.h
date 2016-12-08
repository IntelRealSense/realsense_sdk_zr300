// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/**
* \file v4l_image_factory.h
* @brief Describes the \c rs::core::create_instance_from_v4l_buffer factory method.
*/

#pragma once

#include <linux/videodev2.h>
#include <rs/core/image_interface.h>

namespace rs
{
    namespace core
    {
        /**
         * @brief sdk image creation from a video4linux buffer.
         *
         * sdk image creation from a video4linux buffer, where the user provides an allocated image data and
         * an optional image deallocation method with the data_releaser_interface. if no deallocation method is provided,
         * it assumes that the user is handling memory deallocation outside of the custom image class.
         * @param[in] data_container        The allocated image data and the data releasing handler. The releasing handler release function will be called by
         *                                  The image destructor. A null data_releaser means the user is managing the image data outside of the image instance.
         * @param[in] v4l_buffer_info       A v4l_buffer, which includes the information retrieved by calling VIDIOC_DQBUF
         * @param[in] stream                The sensor type (stream type), which produces the image
         * @param[in] v4l_image_info        The image info, which matches the VIDIOC_G_FMT out parameter
         * @return image_interface *        An image instance.
         */
         image_interface* create_instance_from_v4l_buffer(const image_interface::image_data_with_data_releaser& data_container,
                                                                v4l2_buffer v4l_buffer_info,
                                                                stream_type stream,
                                                                v4l2_pix_format v4l_image_info);
    }
}
