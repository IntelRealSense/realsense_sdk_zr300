// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.


#include "rs/utils/librealsense_conversion_utils.h"
#include "gtest/gtest.h"

using namespace rs::core;
using namespace rs::utils;

GTEST_TEST(librealsense_types_conversion, stream_conversions)
{
    //validate that librealsense keeps rs::stream enum compatibility values
    ASSERT_EQ(0,  static_cast<std::underlying_type<rs::stream>::type>(rs::stream::depth));
    ASSERT_EQ(1,  static_cast<std::underlying_type<rs::stream>::type>(rs::stream::color));
    ASSERT_EQ(2,  static_cast<std::underlying_type<rs::stream>::type>(rs::stream::infrared));
    ASSERT_EQ(3,  static_cast<std::underlying_type<rs::stream>::type>(rs::stream::infrared2));
    ASSERT_EQ(4,  static_cast<std::underlying_type<rs::stream>::type>(rs::stream::fisheye));
    ASSERT_EQ(5,  static_cast<std::underlying_type<rs::stream>::type>(rs::stream::points));
    ASSERT_EQ(6,  static_cast<std::underlying_type<rs::stream>::type>(rs::stream::rectified_color));
    ASSERT_EQ(7,  static_cast<std::underlying_type<rs::stream>::type>(rs::stream::color_aligned_to_depth));
    ASSERT_EQ(8,  static_cast<std::underlying_type<rs::stream>::type>(rs::stream::infrared2_aligned_to_depth));
    ASSERT_EQ(9,  static_cast<std::underlying_type<rs::stream>::type>(rs::stream::depth_aligned_to_color));
    ASSERT_EQ(10, static_cast<std::underlying_type<rs::stream>::type>(rs::stream::depth_aligned_to_rectified_color));
    ASSERT_EQ(11, static_cast<std::underlying_type<rs::stream>::type>(rs::stream::depth_aligned_to_infrared2));
    ASSERT_EQ(static_cast<std::underlying_type<stream_type>::type>(stream_type::max), RS_STREAM_COUNT)
            << "stream count has changed, integrating a new librealsense version?, update the conversion functions";

    //validate that conversion to the librealsense types is valid
    ASSERT_EQ(convert_stream_type(stream_type::depth), rs::stream::depth);
    ASSERT_EQ(convert_stream_type(stream_type::color), rs::stream::color);
    ASSERT_EQ(convert_stream_type(stream_type::infrared), rs::stream::infrared);
    ASSERT_EQ(convert_stream_type(stream_type::infrared2), rs::stream::infrared2);
    ASSERT_EQ(convert_stream_type(stream_type::fisheye), rs::stream::fisheye);
    ASSERT_EQ(convert_stream_type(stream_type::points), rs::stream::points);
    ASSERT_EQ(convert_stream_type(stream_type::rectified_color), rs::stream::rectified_color);
    ASSERT_EQ(convert_stream_type(stream_type::color_aligned_to_depth), rs::stream::color_aligned_to_depth);
    ASSERT_EQ(convert_stream_type(stream_type::infrared2_aligned_to_depth), rs::stream::infrared2_aligned_to_depth);
    ASSERT_EQ(convert_stream_type(stream_type::depth_aligned_to_color), rs::stream::depth_aligned_to_color);
    ASSERT_EQ(convert_stream_type(stream_type::depth_aligned_to_rectified_color), rs::stream::depth_aligned_to_rectified_color);
    ASSERT_EQ(convert_stream_type(stream_type::depth_aligned_to_infrared2), rs::stream::depth_aligned_to_infrared2);

    //validate that conversion to the sdk type is valid
    ASSERT_EQ(convert_stream_type(rs::stream::depth), stream_type::depth);
    ASSERT_EQ(convert_stream_type(rs::stream::color), stream_type::color);
    ASSERT_EQ(convert_stream_type(rs::stream::infrared), stream_type::infrared);
    ASSERT_EQ(convert_stream_type(rs::stream::infrared2), stream_type::infrared2);
    ASSERT_EQ(convert_stream_type(rs::stream::fisheye), stream_type::fisheye);
    ASSERT_EQ(convert_stream_type(rs::stream::points), stream_type::points);
    ASSERT_EQ(convert_stream_type(rs::stream::rectified_color), stream_type::rectified_color);
    ASSERT_EQ(convert_stream_type(rs::stream::color_aligned_to_depth), stream_type::color_aligned_to_depth);
    ASSERT_EQ(convert_stream_type(rs::stream::infrared2_aligned_to_depth), stream_type::infrared2_aligned_to_depth);
    ASSERT_EQ(convert_stream_type(rs::stream::depth_aligned_to_color), stream_type::depth_aligned_to_color);
    ASSERT_EQ(convert_stream_type(rs::stream::depth_aligned_to_rectified_color), stream_type::depth_aligned_to_rectified_color);
    ASSERT_EQ(convert_stream_type(rs::stream::depth_aligned_to_infrared2), stream_type::depth_aligned_to_infrared2);
}
