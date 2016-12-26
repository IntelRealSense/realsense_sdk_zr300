// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.


#include "gtest/gtest.h"
#include <rs/core/context.h>
#include "utilities/version.h"


using namespace std;
using namespace test_utils;

/**
 * @ref https://github.com/IntelRealSense/librealsense/releases
 */
const version required_librealsense_version("1.12.0");
const version required_camera_firmware_version("2.0.71.28");
const version required_adapter_board_firmware_version("1.29.0.0");
const version required_motion_module_firmware_version("1.25.0.0");


GTEST_TEST(firmware_version_tests, zr300_fw_versions)
{
    rs::core::context context;
    ASSERT_NE(context.get_device_count(), 0) << "No camera is connected";
    
    rs::device * device = context.get_device(0);
    
    std::cout << "Device Name : " << device->get_info(rs::camera_info::device_name) << std::endl;
    
    std::string camera_firmware_version_str(device->get_info(rs::camera_info::camera_firmware_version));
    std::cout << "Camera Firmware Version : " << camera_firmware_version_str << std::endl;
    version camera_firmware_version;
    ASSERT_NO_THROW(camera_firmware_version = version(camera_firmware_version_str));
    EXPECT_EQ(camera_firmware_version, required_camera_firmware_version) << "Camera Firmware Version is different than the required FW";
    
    std::string adapter_board_firmware_version_str(device->get_info(rs::camera_info::adapter_board_firmware_version));
    std::cout << "Adapter Board Firmware Version : " << adapter_board_firmware_version_str << std::endl;
    version adapter_board_firmware_version;
    ASSERT_NO_THROW(adapter_board_firmware_version = version(adapter_board_firmware_version_str));
    EXPECT_EQ(adapter_board_firmware_version, required_adapter_board_firmware_version) << "Adapter Board Firmware Version is different than the required FW";
    
    std::string motion_module_firmware_version_str(device->get_info(rs::camera_info::motion_module_firmware_version));
    std::cout << "Motion Module Firmware Version : " << motion_module_firmware_version_str << std::endl;
    version motion_module_firmware_version;
    ASSERT_NO_THROW(motion_module_firmware_version = version(motion_module_firmware_version_str));
    EXPECT_EQ(motion_module_firmware_version, required_motion_module_firmware_version) << "Motion Module Firmware Version is different than the required FW";
}

GTEST_TEST(librealsense_version_tests, librs_version)
{
    version current_librealsense_version(RS_API_VERSION_STR);
    std::cout << "LibRealSense Version is: " << current_librealsense_version << std::endl;
    ASSERT_EQ(current_librealsense_version, required_librealsense_version);
}