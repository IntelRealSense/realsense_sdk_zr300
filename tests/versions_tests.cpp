// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <algorithm>
#include <map>
#include <fstream>

#include "gtest/gtest.h"
#include <rs/core/context.h>
#include "utilities/version.h"
#include "utilities/utilities.h"

using namespace test_utils;

class versions_tests : public testing::Test
{
protected:
    
	static constexpr const char* dependencies_versions_file_name = "dependencies_versions";
    static std::map<std::string, version> required_versions;
    static std::string current_executable_path()
    {
        char curr_exe_path[1024];
        if(readlink("/proc/self/exe", curr_exe_path, 1024) <= 0)
        {
            return "";
        }
        std::string curr_dir(curr_exe_path);
        return curr_dir.substr(0, curr_dir.find_last_of("/"));
    }
    // Sets up the test fixture.
    static void SetUpTestCase()
    {
        std::string current_exe_path(current_executable_path());
        ASSERT_STRNE(current_exe_path.c_str(), "") << "Failed to find current executable path";
        std::cout << "Current executable path is: " << current_exe_path << std::endl;
        std::string path_to_versions_file = current_exe_path + std::string("/") + dependencies_versions_file_name ;
        std::ifstream versions_file(path_to_versions_file); //Cmake should have copied this file here
        ASSERT_TRUE(versions_file.good()) << "Failed to open file " << path_to_versions_file;
        
        std::string line;
        while (std::getline(versions_file, line))
        {
            //remove whitespaces
            auto removed = std::remove_if(line.begin(), line.end(), ::isspace);
            line.erase(removed, line.end());

            std::string key;
            std::istringstream iss_line(line);
            if (std::getline(iss_line, key, '='))
            {
                if (key[0] == '#') //comment line
                {
                    continue;
                }
                std::string value;
                if (std::getline(iss_line, value))
                {
                    try
                    {
                        versions_tests::required_versions[key] = version(value);
                    }
                    catch(std::exception e)
                    {
                        std::cerr << "Failed to create version of " << key << "with value of " << value << std::endl;
                    }
                }
            }
        }
    }
};

std::map<std::string, version> versions_tests::required_versions;

TEST_F(versions_tests, zr300_firmware_version_tests)
{
    rs::core::context context;
    int device_count = context.get_device_count();
    ASSERT_NE(device_count, 0) << "No camera is connected";
    rs::device * device = nullptr;
    for(int i = 0; i < device_count; i++)
    {
        device = context.get_device(i);
        ASSERT_NE(device, nullptr) << "Failed to get device at " << i;
        std::cout << "Device Name : " << device->get_info(rs::camera_info::device_name) << std::endl;
        if(strcmp(device->get_name(), "Intel RealSense ZR300") == 0)
        {
            std::string camera_firmware_version_str(device->get_info(rs::camera_info::camera_firmware_version));
            std::cout << "Camera Firmware Version : " << camera_firmware_version_str << std::endl;
            version camera_firmware_version;
            ASSERT_NO_THROW(camera_firmware_version = version(camera_firmware_version_str));
            EXPECT_EQ(camera_firmware_version, versions_tests::required_versions.at("zr300_camera_firmware_version")) << "Camera Firmware Version is different than the required FW";
    
            std::string adapter_board_firmware_version_str(device->get_info(rs::camera_info::adapter_board_firmware_version));
            std::cout << "Adapter Board Firmware Version : " << adapter_board_firmware_version_str << std::endl;
            version adapter_board_firmware_version;
            ASSERT_NO_THROW(adapter_board_firmware_version = version(adapter_board_firmware_version_str));
            EXPECT_EQ(adapter_board_firmware_version, versions_tests::required_versions.at("zr300_adapter_board_firmware_version")) << "Adapter Board Firmware Version is different than the required FW";
    
            std::string motion_module_firmware_version_str(device->get_info(rs::camera_info::motion_module_firmware_version));
            std::cout << "Motion Module Firmware Version : " << motion_module_firmware_version_str << std::endl;
            version motion_module_firmware_version;
            ASSERT_NO_THROW(motion_module_firmware_version = version(motion_module_firmware_version_str));
            EXPECT_EQ(motion_module_firmware_version, versions_tests::required_versions.at("zr300_motion_module_firmware_version")) << "Motion Module Firmware Version is different than the required FW";
        }
        device = nullptr;
    }
}

TEST_F(versions_tests, librs_version)
{
    version current_librealsense_version(RS_API_VERSION_STR);
    std::cout << "LibRealSense Version is: " << current_librealsense_version << std::endl;
    ASSERT_EQ(current_librealsense_version, versions_tests::required_versions.at("librealsense_version"));
}