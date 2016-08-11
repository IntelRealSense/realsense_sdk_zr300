// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "image_base.h"
#include "custom_image.h"
#include "image_conversion_util.h"
#include "image_utils.h"

using namespace rs::utils;

namespace rs
{
    namespace core
    {

        class data_releaser_impl : public image_interface::data_releaser_interface
        {
        public:
            data_releaser_impl(uint8_t* data) :data(data) {}
            void release() { delete [] data; }
        private:
            uint8_t* data;
        };

        image_base::image_base(smart_ptr<metadata_interface> metadata)
            : metadata(metadata) {}

        smart_ptr<metadata_interface> image_base::query_metadata()
        {
            return metadata;
        }

        status image_base::convert_to(pixel_format format, smart_ptr<const image_interface> &converted_image)
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
                smart_ptr<image_interface::data_releaser_interface> data_releaser(new data_releaser_impl(dst_data));

                // update the dst image data
                if(image_conversion_util::convert(query_info(), static_cast<const uint8_t *>(query_data()), dst_info, dst_data) < status_no_error)
                {
                    data_releaser->release();
                    return status_param_unsupported;
                }

                //cache the image
                const image_interface * dst_image = create_instance_from_raw_data(&dst_info,
                        dst_data,
                        query_stream_type(),
                        query_flags(),
                        query_time_stamp(),
                        query_frame_number(),
                        query_metadata(),
                        data_releaser);

                image_cache_per_pixel_format[format] = smart_ptr<const image_interface>(dst_image);
            }
            converted_image = image_cache_per_pixel_format[format];
            return status_no_error;
        }

        status image_base::convert_to(rs::core::rotation rotation, smart_ptr<const image_interface> &converted_image)
        {
            return status_feature_unsupported;
        }
    }
}


