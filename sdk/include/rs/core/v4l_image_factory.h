// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once

#include <linux/videodev2.h>
#include <rs/core/image_interface.h>

namespace rs
{
    namespace core
    {
        /**
         * @brief create_instance_from_v4l_buffer
         *
         * sdk image creation from a video4linux buffer, where the user provides an allocated image data and
         * an optional image deallocation method with the data_releaser_interface. if no deallocation method is provided,
         * it assumes that the user is handling memory deallocation outside of the custom image class.
         * @param[in] data_container        the allocated image data and the data releasing handler. The releasing handler release function will be called by
         *                                  the image destructor. A null data_releaser means the user is managing the image data outside of the image instance.
         * @param[in] v4l_buffer_info       a v4l_buffer, which includes the information retrieved by calling VIDIOC_DQBUF
         * @param[in] stream                the sensor type (stream type), which produces the image
         * @param[in] v4l_image_info        the image info, which matches the VIDIOC_G_FMT out parameter
         * @return image_interface *    an image instance.
         */
         image_interface* create_instance_from_v4l_buffer(const image_interface::image_data_with_data_releaser& data_container,
                                                                v4l2_buffer v4l_buffer_info,
                                                                stream_type stream,
                                                                v4l2_pix_format v4l_image_info);
    }
}
