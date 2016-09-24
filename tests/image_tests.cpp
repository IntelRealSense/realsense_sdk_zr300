// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <limits.h>
#include <iostream>
#include <memory>
#include "gtest/gtest.h"
#include "utilities/utilities.h"
#include "librealsense/rs.hpp"
#include "image/librealsense_image_utils.h"
#include "rs/utils/librealsense_conversion_utils.h"
#include "viewer.h"
#include <chrono>

using namespace std;
using namespace rs::core;
using namespace rs::utils;


struct conversion_test_data
{
	rs::stream stream;
	image_info src_info;
	image_info dst_info;
	conversion_test_data(rs::stream stream, image_info src_info, image_info dst_info) :stream(stream), src_info(src_info), dst_info(dst_info) {}
};

::std::ostream& operator<<(::std::ostream& os, const image_info& info)
{
	return os << info.width << "x" << info.height << " " << convert_pixel_format(info.format);
}

::std::ostream& operator<<(::std::ostream& os, const conversion_test_data& test_data)
{
	return os << "stream:" << test_data.stream << ",src:" << test_data.src_info << ",dst:" << test_data.dst_info;
}

class image_conversions_tests : public testing::TestWithParam<conversion_test_data>
{
public:
	rs::context m_contex;
	rs::device * m_device;

	static image_info get_info(int width, int height, rs::format format)
	{
		return{ width, height, convert_pixel_format(format), width *  image_utils::get_pixel_size(format) };
	}

	virtual void SetUp()
	{
		ASSERT_NE(m_contex.get_device_count(), 0) << "No camera is connected";
		m_device = m_contex.get_device(0);
	}

	virtual void TearDown()
	{
		m_device = nullptr;
	}
};

TEST_P(image_conversions_tests, check_supported_conversions)
{
	conversion_test_data test_data = GetParam();

	m_device->enable_stream(test_data.stream, test_data.src_info.width, test_data.src_info.height, convert_pixel_format(test_data.src_info.format), 30);
	if (test_data.stream == rs::stream::infrared)//this will turn on the projector
		m_device->set_option(rs::option::r200_emitter_enabled, 1);
	m_device->start();
	m_device->wait_for_frames();
	std::this_thread::sleep_for(std::chrono::seconds(1));
	m_device->wait_for_frames();

	auto image = get_unique_ptr_with_releaser(image_interface::create_instance_from_raw_data(&test_data.src_info,
	{ m_device->get_frame_data(test_data.stream), nullptr },
		convert_stream_type(test_data.stream),
		image_interface::flag::any,
		m_device->get_frame_timestamp(test_data.stream),
		m_device->get_frame_number(test_data.stream),
		nullptr));

	const image_interface * raw_converted_image = nullptr;
	ASSERT_EQ(status_no_error, image->convert_to(test_data.dst_info.format, &raw_converted_image)) << "failed to convert image";
	auto converted_image = get_unique_ptr_with_releaser(raw_converted_image);
	ASSERT_NE(nullptr, converted_image) << "converted image is null";
	ASSERT_NE(nullptr, converted_image->query_data()) << "converted image doesn't have data";
	auto info = converted_image->query_info();
	ASSERT_EQ(test_data.dst_info.format, info.format) << "converted image not in the right format";
	std::ostringstream display_title;
	display_title << "converted : " << image->query_info() << " to : " << converted_image->query_info();

	auto viewer = std::make_shared<rs::utils::viewer>(1, 640, nullptr, display_title.str());

	converted_image->add_ref();
	viewer->show_image(converted_image.get());
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	//check caching with another convert request
	const image_interface * raw_second_converted_image = nullptr;
	ASSERT_EQ(status_no_error, image->convert_to(test_data.dst_info.format, &raw_second_converted_image)) << "failed to convert second image";
	auto second_converted_image = get_unique_ptr_with_releaser(raw_second_converted_image);
	ASSERT_NE(nullptr, second_converted_image) << "converted image is null";
	ASSERT_EQ(converted_image->query_data(), second_converted_image->query_data()) << "the converted image wasnt cached";
}

INSTANTIATE_TEST_CASE_P(basic_conversions, image_conversions_tests, ::testing::Values(
	// bug in librealsens for profile infrared y8
	//    conversion_test_data(rs::stream::infrared, image_conversions_tests::get_info(640, 480, rs::format::y8 ), image_conversions_tests::get_info(640, 480, rs::format::bgr8)),
	//    conversion_test_data(rs::stream::infrared, image_conversions_tests::get_info(640, 480, rs::format::y8 ), image_conversions_tests::get_info(640, 480, rs::format::rgb8)),
	//    conversion_test_data(rs::stream::infrared, image_conversions_tests::get_info(640, 480, rs::format::y8 ), image_conversions_tests::get_info(640, 480, rs::format::rgba8)),
	//    conversion_test_data(rs::stream::infrared, image_conversions_tests::get_info(640, 480, rs::format::y8 ), image_conversions_tests::get_info(640, 480, rs::format::bgra8)),

	conversion_test_data(rs::stream::infrared, image_conversions_tests::get_info(640, 480, rs::format::y16), image_conversions_tests::get_info(640, 480, rs::format::bgr8)),
	conversion_test_data(rs::stream::infrared, image_conversions_tests::get_info(628, 468, rs::format::y16), image_conversions_tests::get_info(640, 480, rs::format::rgb8)),
	conversion_test_data(rs::stream::infrared, image_conversions_tests::get_info(480, 360, rs::format::y16), image_conversions_tests::get_info(640, 480, rs::format::rgba8)),
	conversion_test_data(rs::stream::infrared, image_conversions_tests::get_info(320, 240, rs::format::y16), image_conversions_tests::get_info(640, 480, rs::format::bgra8)),

	conversion_test_data(rs::stream::depth, image_conversions_tests::get_info(320, 240, rs::format::z16), image_conversions_tests::get_info(640, 480, rs::format::bgr8)),
	conversion_test_data(rs::stream::depth, image_conversions_tests::get_info(628, 468, rs::format::z16), image_conversions_tests::get_info(640, 480, rs::format::rgb8)),
	conversion_test_data(rs::stream::depth, image_conversions_tests::get_info(640, 480, rs::format::z16), image_conversions_tests::get_info(640, 480, rs::format::rgba8)),
	conversion_test_data(rs::stream::depth, image_conversions_tests::get_info(480, 360, rs::format::z16), image_conversions_tests::get_info(640, 480, rs::format::bgra8)),

	conversion_test_data(rs::stream::color, image_conversions_tests::get_info(1920, 1080, rs::format::bgr8), image_conversions_tests::get_info(640, 480, rs::format::y8)),
	conversion_test_data(rs::stream::color, image_conversions_tests::get_info(640, 480, rs::format::bgr8), image_conversions_tests::get_info(640, 480, rs::format::rgb8)),
	conversion_test_data(rs::stream::color, image_conversions_tests::get_info(1920, 1080, rs::format::bgr8), image_conversions_tests::get_info(640, 480, rs::format::rgba8)),
	conversion_test_data(rs::stream::color, image_conversions_tests::get_info(640, 480, rs::format::bgr8), image_conversions_tests::get_info(640, 480, rs::format::bgra8)),

	conversion_test_data(rs::stream::color, image_conversions_tests::get_info(1920, 1080, rs::format::rgb8), image_conversions_tests::get_info(640, 480, rs::format::y8)),
	conversion_test_data(rs::stream::color, image_conversions_tests::get_info(640, 480, rs::format::rgb8), image_conversions_tests::get_info(640, 480, rs::format::bgr8)),
	conversion_test_data(rs::stream::color, image_conversions_tests::get_info(1920, 1080, rs::format::rgb8), image_conversions_tests::get_info(640, 480, rs::format::rgba8)),
	conversion_test_data(rs::stream::color, image_conversions_tests::get_info(640, 480, rs::format::rgb8), image_conversions_tests::get_info(640, 480, rs::format::bgra8)),

	conversion_test_data(rs::stream::color, image_conversions_tests::get_info(640, 480, rs::format::rgba8), image_conversions_tests::get_info(640, 480, rs::format::y8)),
	conversion_test_data(rs::stream::color, image_conversions_tests::get_info(1920, 1080, rs::format::rgba8), image_conversions_tests::get_info(640, 480, rs::format::bgr8)),
	conversion_test_data(rs::stream::color, image_conversions_tests::get_info(640, 480, rs::format::rgba8), image_conversions_tests::get_info(640, 480, rs::format::rgb8)),
	conversion_test_data(rs::stream::color, image_conversions_tests::get_info(1920, 1080, rs::format::rgba8), image_conversions_tests::get_info(640, 480, rs::format::bgra8)),

	conversion_test_data(rs::stream::color, image_conversions_tests::get_info(640, 480, rs::format::bgra8), image_conversions_tests::get_info(640, 480, rs::format::y8)),
	conversion_test_data(rs::stream::color, image_conversions_tests::get_info(1920, 1080, rs::format::bgra8), image_conversions_tests::get_info(640, 480, rs::format::bgr8)),
	conversion_test_data(rs::stream::color, image_conversions_tests::get_info(640, 480, rs::format::bgra8), image_conversions_tests::get_info(640, 480, rs::format::rgb8)),
	conversion_test_data(rs::stream::color, image_conversions_tests::get_info(1920, 1080, rs::format::bgra8), image_conversions_tests::get_info(640, 480, rs::format::rgba8)),

	conversion_test_data(rs::stream::color, image_conversions_tests::get_info(1920, 1080, rs::format::yuyv), image_conversions_tests::get_info(640, 480, rs::format::y8)),
	conversion_test_data(rs::stream::color, image_conversions_tests::get_info(640, 480, rs::format::yuyv), image_conversions_tests::get_info(640, 480, rs::format::bgr8)),
	conversion_test_data(rs::stream::color, image_conversions_tests::get_info(1920, 1080, rs::format::yuyv), image_conversions_tests::get_info(640, 480, rs::format::rgb8)),
	conversion_test_data(rs::stream::color, image_conversions_tests::get_info(640, 480, rs::format::yuyv), image_conversions_tests::get_info(640, 480, rs::format::rgba8)),
	conversion_test_data(rs::stream::color, image_conversions_tests::get_info(1920, 1080, rs::format::yuyv), image_conversions_tests::get_info(640, 480, rs::format::bgra8))
	));


