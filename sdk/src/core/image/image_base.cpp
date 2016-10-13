// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "image_base.h"
#include "custom_image.h"
#include "image_conversion_util.h"
#include "image_utils.h"
#include "rs/utils/self_releasing_array_data_releaser.h"
#include "rs/utils/smart_ptr_helpers.h"
#include "rs_sdk_version.h"

namespace rs
{
    namespace core
    {
        image_base::image_base(metadata_interface * metadata)
            : ref_count_base(), metadata(rs::utils::get_unique_ptr_with_releaser(metadata)) {}

        metadata_interface * image_base::query_metadata()
        {
            if(metadata)
            {
                metadata->add_ref();
            }

            return metadata.get();
        }

        status image_base::convert_to(pixel_format format, const image_interface **converted_image)
        {
            image_info dst_info = query_info();
            dst_info.format = format;
            dst_info.pitch = image_utils::get_pixel_size(format) * query_info().width;

            if(image_conversion_util::is_conversion_valid(query_info(), dst_info) < status_no_error)
            {
                return status_param_unsupported;
            }

            std::lock_guard<std::mutex> lock(image_caching_lock);
            if(image_cache_per_pixel_format.find(format) == image_cache_per_pixel_format.end())
            {
                //allocate image data
                auto dst_data = new uint8_t[dst_info.height * dst_info.pitch];

                //create a releaser for the above allocation
                auto data_releaser = new rs::utils::self_releasing_array_data_releaser(dst_data);

                // update the dst image data
                if(image_conversion_util::convert(query_info(), static_cast<const uint8_t *>(query_data()), dst_info, dst_data) < status_no_error)
                {
                    data_releaser->release();
                    return status_param_unsupported;
                }

                //cache the image
                const image_interface * dst_image = image_interface::create_instance_from_raw_data(
                        &dst_info,
                        {dst_data, data_releaser},
                        query_stream_type(),
                        query_flags(),
                        query_time_stamp(),
                        query_frame_number(),
                        query_metadata());

                image_cache_per_pixel_format[format] = rs::utils::get_unique_ptr_with_releaser(dst_image);
            }
            image_cache_per_pixel_format[format]->add_ref();
            *converted_image = image_cache_per_pixel_format[format].get();
            return status_no_error;
        }

        status image_base::convert_to(rs::core::rotation rotation, const image_interface **converted_image)
        {
            return status_feature_unsupported;
        }
    }
}


