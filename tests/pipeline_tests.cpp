// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <thread>
#include <fstream>
#include <cstring>
#include <algorithm>

#include "gtest/gtest.h"
#include "rs_sdk.h"
#include "../sdk/src/cv_modules/max_depth_value_module/max_depth_value_module_impl.h"
#include "../sdk/src/core/pipeline/config_util.h"

using namespace std;
using namespace rs::core;
using namespace rs::utils;
using namespace rs::cv_modules;

class max_depth_value_module_testing : public max_depth_value_module_impl
{
public:
    max_depth_value_module_testing(): m_is_using_custom_config(false), m_supported_config({})
    {}

    video_module_interface::supported_module_config::time_sync_mode query_time_sync_mode()
    {
        return m_time_sync_mode;
    }

    void set_module_uid(int32_t unique_module_id)
    {
        m_unique_module_id = unique_module_id;
    }

    void set_processing_mode(bool is_async_processing)
    {
        m_async_processing = is_async_processing;
    }

    void set_custom_configs(supported_module_config &supported_config)
    {
        m_is_using_custom_config = true;
        m_supported_config = supported_config;
    }

    status query_supported_module_config(int32_t idx, supported_module_config &supported_config)
    {
        if(!m_is_using_custom_config)
        {
            return max_depth_value_module_impl::query_supported_module_config(idx, supported_config);
        }

        //validate input index
        if(idx != 0)
        {
            return status_item_unavailable;
        }

        supported_config = m_supported_config;
        return status_no_error;
    }

    status process_sample_set(const correlated_sample_set& sample_set)
    {
        if(!m_is_using_custom_config)
        {
            return max_depth_value_module_impl::process_sample_set(sample_set);
        }

        //check sample sets are time synced according to the supported configuration
        if(m_supported_config.samples_time_sync_mode == video_module_interface::supported_module_config::time_sync_mode::time_synced_input_only)
        {
            for(auto stream_index = 0; stream_index < static_cast<int32_t>(stream_type::max); stream_index++)
            {
                auto stream = static_cast<stream_type>(stream_index);
                if(m_supported_config[stream].is_enabled)
                {
                    image_interface * image = sample_set[stream];
                    EXPECT_TRUE(image != nullptr) << "expected in configuration mode "<< static_cast<int>(m_supported_config.samples_time_sync_mode)<<
                                                     " to get sample sets with stream type " << stream_index;
                }
            }
        }

        auto depth_image = sample_set[stream_type::depth];
        if(!depth_image)
        {
            return status_item_unavailable;
        }

        max_depth_value_output_interface::max_depth_value_output_data output_data;
        depth_image->add_ref();
        auto status = process_depth_max_value(get_unique_ptr_with_releaser(depth_image), output_data);
        if(status < status_no_error)
        {
            return status;
        }

        m_output_data.set(output_data);
        return status_no_error;
    }

private:
    bool m_is_using_custom_config;
    supported_module_config m_supported_config;
};


class pipeline_handler : public pipeline_async_interface::callback_handler
{
public:
    pipeline_handler(const std::shared_ptr<max_depth_value_module_testing> & max_depth_value_module) :
        m_was_a_new_valid_sample_dispatched(false),
        m_was_a_new_max_depth_value_dispatched(false),
        m_max_depth_value_module(max_depth_value_module)
    {}

    void on_new_sample_set(const correlated_sample_set& sample_set) override
    {
        auto depth_image = sample_set.get_unique(stream_type::depth);
        video_module_interface::actual_module_config actual_config = {};
        m_max_depth_value_module->query_current_module_config(actual_config);
        if(actual_config[stream_type::depth].is_enabled &&
                m_max_depth_value_module->query_time_sync_mode() == video_module_interface::supported_module_config::time_sync_mode::time_synced_input_only)
        {
            ASSERT_TRUE(depth_image != nullptr) << "got null depth image";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        m_was_a_new_valid_sample_dispatched = true;
    }

    void on_cv_module_process_complete(video_module_interface * cv_module) override
    {
        ASSERT_EQ(m_max_depth_value_module->query_module_uid(), cv_module->query_module_uid()) << "the module id is wrong";

        auto max_depth_data = m_max_depth_value_module->get_max_depth_value_data();
        ASSERT_TRUE(max_depth_data.frame_number > 0)<<"the frame number supposed to be larger than 0";

        m_was_a_new_max_depth_value_dispatched = true;
    }

    void on_error(status status) {}

    bool was_a_new_valid_sample_dispatched()
    {
        return m_was_a_new_valid_sample_dispatched;
    }

    bool was_a_new_max_depth_value_dispatched()
    {
        return m_was_a_new_max_depth_value_dispatched;
    }

private:
    std::shared_ptr<max_depth_value_module_testing> m_max_depth_value_module;
    bool m_was_a_new_valid_sample_dispatched;
    bool m_was_a_new_max_depth_value_dispatched;
};

class pipeline_tests : public testing::Test
{
protected:
    pipeline_tests(): m_module(nullptr), m_pipeline(nullptr) {}
    std::unique_ptr<pipeline_handler> m_callback_handler;
    std::shared_ptr<max_depth_value_module_testing> m_module;
    std::unique_ptr<pipeline_async_interface> m_pipeline;

    virtual void SetUp()
    {
        m_module.reset(new max_depth_value_module_testing());
        m_callback_handler.reset(new pipeline_handler(m_module));
        m_pipeline.reset(new pipeline_async());
    }
    virtual void TearDown()
    {
        m_pipeline.reset();
        m_callback_handler.reset();
        m_module.reset();
    }
};

TEST_F(pipeline_tests, add_cv_module)
{
    ASSERT_EQ(status_data_not_initialized, m_pipeline->add_cv_module(nullptr)) <<"add_cv_module with null didnt fail";
    ASSERT_EQ(status_no_error, m_pipeline->add_cv_module(m_module.get())) <<"failed to add cv module to pipeline";
    ASSERT_EQ(status_param_inplace, m_pipeline->add_cv_module(m_module .get())) << "double adding the same cv module didnt fail";
}

TEST_F(pipeline_tests, query_cv_module)
{
    ASSERT_EQ(status_value_out_of_range, m_pipeline->query_cv_module(0, nullptr)) << "no modules should should output out of range index";
    ASSERT_EQ(status_value_out_of_range, m_pipeline->query_cv_module(-1, nullptr)) << "query_cv_module failed to treat out of range index";

    m_pipeline->add_cv_module(m_module.get());

    ASSERT_EQ(status_handle_invalid, m_pipeline->query_cv_module(0, nullptr)) << "query_cv_module failed to treat null ptr to ptr initialization";
    ASSERT_EQ(status_value_out_of_range, m_pipeline->query_cv_module(-1, nullptr)) << "query_cv_module failed to treat out of range index";

    video_module_interface * queried_module = nullptr;
    ASSERT_EQ(status_no_error, m_pipeline->query_cv_module(0, &queried_module))<< "failed to query cv module";
    ASSERT_EQ(queried_module->query_module_uid(), m_module->query_module_uid())<< "first module should be the original module";
}

TEST_F(pipeline_tests, query_default_config)
{
    video_module_interface::supported_module_config available_config= {};
    ASSERT_EQ(status_value_out_of_range, m_pipeline->query_default_config(-1, available_config))<<"fail on wrong index";
    ASSERT_EQ(status_no_error, m_pipeline->query_default_config(0, available_config))<<"failed to query index 0 available config, without cv modules";
    m_pipeline->add_cv_module(m_module.get());
    ASSERT_EQ(status_no_error, m_pipeline->query_default_config(0, available_config))<<"failed to query index 0 available config, with cv module";
}

TEST_F(pipeline_tests, set_config)
{
    video_module_interface::supported_module_config config = {};
    ASSERT_EQ(status_invalid_argument, m_pipeline->set_config(config))<<"unavailable config should fail";

    config[stream_type::color].is_enabled = true;
    ASSERT_EQ(status_no_error, m_pipeline->set_config(config))<<"failed set config without module and with a valid stream";

    m_pipeline->reset();

    m_pipeline->add_cv_module(m_module.get());
    ASSERT_EQ(status_no_error, m_pipeline->set_config(config))<<"failed set config when a valid module added";
}

TEST_F(pipeline_tests, query_current_config)
{
    video_module_interface::actual_module_config current_config = {};
    ASSERT_EQ(status_invalid_state, m_pipeline->query_current_config(current_config));

    video_module_interface::supported_module_config config = {};
    config[stream_type::color].is_enabled = true;
    m_pipeline->set_config(config);

    current_config = {};
    ASSERT_EQ(status_no_error, m_pipeline->query_current_config(current_config)) << "failed to query current configuration";
    ASSERT_EQ(true, current_config[stream_type::color].is_enabled) << "current config was enabled with color stream";
    ASSERT_NE(0, current_config[stream_type::color].size.width) << "pipeline should have filled the missing configuration data";
    m_pipeline->start(m_callback_handler.get());
    ASSERT_EQ(status_no_error, m_pipeline->query_current_config(current_config));
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    m_pipeline->stop();

    m_pipeline->reset();

    m_pipeline->add_cv_module(m_module.get());
    config = {};
    config[stream_type::color].is_enabled = true;
    m_pipeline->set_config(config);
    current_config = {};
    ASSERT_EQ(status_no_error, m_pipeline->query_current_config(current_config)) << "failed to query current configuration";
    ASSERT_EQ(true, current_config[stream_type::color].is_enabled) << "current config was enabled with color stream due to manual user configuration";
    ASSERT_NE(0, current_config[stream_type::color].size.width) << "pipeline should have filled the missing configuration data";
    ASSERT_EQ(true, current_config[stream_type::depth].is_enabled) << "current config was enabled with depth stream due to the module configuration";
    ASSERT_NE(0, current_config[stream_type::depth].size.width) << "pipeline should have filled the missing configuration data";
}

TEST_F(pipeline_tests, reset)
{
    m_pipeline->add_cv_module(m_module.get());
    ASSERT_EQ(status_no_error, m_pipeline->reset());
    ASSERT_NE(status_param_inplace, m_pipeline->add_cv_module(m_module.get())) << "reset should clear the modules from the pipeline";

    m_pipeline->add_cv_module(m_module.get());
    m_pipeline->start(m_callback_handler.get());
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    ASSERT_EQ(status_no_error, m_pipeline->reset());
    m_callback_handler.reset(new pipeline_handler(m_module));
    ASSERT_NE(status_no_error, m_pipeline->start(m_callback_handler.get())) <<"the pipeline expects a new configuration";
    m_pipeline->add_cv_module(m_module.get());
    ASSERT_EQ(status_no_error, m_pipeline->start(m_callback_handler.get()));
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    ASSERT_EQ(status_no_error, m_pipeline->stop());
}

TEST_F(pipeline_tests, get_device)
{
    m_pipeline->add_cv_module(m_module.get());
    ASSERT_EQ(nullptr, m_pipeline->get_device())<<"the pipeline is unconfigured, should have null device handle";
    video_module_interface::supported_module_config supported_config = {};
    m_module->query_supported_module_config(0, supported_config);
    ASSERT_EQ(status_no_error, m_pipeline->set_config(supported_config));
    ASSERT_NE(nullptr, m_pipeline->get_device())<<"the pipeline is configured, should have a valid device handle";
    ASSERT_EQ(status_no_error, m_pipeline->start(nullptr));
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    ASSERT_NE(nullptr, m_pipeline->get_device())<<"the pipeline is streaming, should have a valid device handle";
    ASSERT_EQ(status_no_error, m_pipeline->stop());
    ASSERT_EQ(status_no_error, m_pipeline->reset());
    ASSERT_EQ(nullptr, m_pipeline->get_device())<<"the pipeline is unconfigured after reset, should have null device handle";
}

TEST_F(pipeline_tests, stream_without_adding_cv_modules_and_with_setting_config)
{
    video_module_interface::supported_module_config config = {};
    config[stream_type::color].is_enabled = true;
    m_pipeline->set_config(config);
    ASSERT_EQ(status_no_error, m_pipeline->start(m_callback_handler.get()));
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_TRUE(m_callback_handler->was_a_new_valid_sample_dispatched()) <<"new valid sample wasn't dispatched";
    ASSERT_EQ(status_no_error, m_pipeline->stop());
}

TEST_F(pipeline_tests, stream_after_adding_cv_modules_and_without_setting_config)
{
    m_pipeline->add_cv_module(m_module.get());
    ASSERT_EQ(status_no_error, m_pipeline->start(m_callback_handler.get()));
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_TRUE(m_callback_handler->was_a_new_valid_sample_dispatched()) <<"new valid sample wasn't dispatched";
    EXPECT_TRUE(m_callback_handler->was_a_new_max_depth_value_dispatched()) <<"new valid cv module output wasn't dispatched";
    ASSERT_EQ(status_no_error, m_pipeline->stop());
}

TEST_F(pipeline_tests, stream_after_adding_cv_modules_and_with_setting_config)
{
    m_pipeline->add_cv_module(m_module.get());
    video_module_interface::supported_module_config config = {};
    config[stream_type::color].is_enabled = true;
    m_pipeline->set_config(config);
    ASSERT_EQ(status_no_error, m_pipeline->start(m_callback_handler.get()));
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_TRUE(m_callback_handler->was_a_new_valid_sample_dispatched()) <<"new valid sample wasn't dispatched";
    EXPECT_TRUE(m_callback_handler->was_a_new_max_depth_value_dispatched()) <<"new valid cv module output wasn't dispatched";
    ASSERT_EQ(status_no_error, m_pipeline->stop());
}

TEST_F(pipeline_tests, async_start_stop_start_stop)
{
    m_pipeline->add_cv_module(m_module.get());
    ASSERT_EQ(status_no_error, m_pipeline->start(m_callback_handler.get()));
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    ASSERT_TRUE(m_callback_handler->was_a_new_valid_sample_dispatched()) <<"new valid sample wasn't dispatched";
    ASSERT_TRUE(m_callback_handler->was_a_new_max_depth_value_dispatched()) <<"new valid cv module output wasn't dispatched";
    ASSERT_EQ(status_no_error, m_pipeline->stop());
    m_callback_handler.reset(new pipeline_handler(m_module));
    ASSERT_EQ(status_no_error, m_pipeline->start(m_callback_handler.get()));
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    ASSERT_TRUE(m_callback_handler->was_a_new_valid_sample_dispatched()) <<"new valid sample wasn't dispatched";
    ASSERT_TRUE(m_callback_handler->was_a_new_max_depth_value_dispatched()) <<"new valid cv module output wasn't dispatched";
    ASSERT_EQ(status_no_error, m_pipeline->stop());
}

TEST_F(pipeline_tests, get_device_and_set_properties)
{
    m_pipeline->add_cv_module(m_module.get());
    video_module_interface::supported_module_config config = {};
    config[stream_type::depth].is_enabled = true;

    m_pipeline->set_config(config);
    auto device = m_pipeline->get_device();

    ASSERT_NO_THROW(device->set_option(rs::option::fisheye_strobe, 1))<<"set option throw exception";
    ASSERT_NO_THROW(device->set_option(rs::option::r200_lr_auto_exposure_enabled, 1))<<"set option throw exception";
    ASSERT_NO_THROW(device->set_option(rs::option::fisheye_color_auto_exposure, 1))<<"set option throw exception";
}

//pending fix from librealsense
TEST_F(pipeline_tests, DISABLED_async_start_and_immediately_stop)
{
    m_pipeline->start(m_callback_handler.get());
    ASSERT_EQ(status_no_error, m_pipeline->stop());
    m_pipeline->start(m_callback_handler.get());
    ASSERT_EQ(status_no_error, m_pipeline->stop());
}

TEST_F(pipeline_tests, check_async_module_is_outputing_data)
{
    bool is_async_processing = true;
    m_module->set_processing_mode(is_async_processing);
    m_pipeline->add_cv_module(m_module.get());
    ASSERT_EQ(status_no_error, m_pipeline->start(m_callback_handler.get()));
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    ASSERT_TRUE(m_callback_handler->was_a_new_max_depth_value_dispatched()) <<"new valid cv module output wasn't dispatched";
    ASSERT_EQ(status_no_error, m_pipeline->stop());
}

TEST_F(pipeline_tests, check_sync_module_is_outputing_data)
{
    bool is_async_processing = false;
    m_module->set_processing_mode(is_async_processing);
    m_pipeline->add_cv_module(m_module.get());
    ASSERT_EQ(status_no_error, m_pipeline->start(m_callback_handler.get()));
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_TRUE(m_callback_handler->was_a_new_max_depth_value_dispatched()) <<"new valid cv module output wasn't dispatched";
    ASSERT_EQ(status_no_error, m_pipeline->stop());
}

TEST_F(pipeline_tests, check_sync_module_gets_time_synced_inputs)
{
    video_module_interface::supported_module_config supported_config = {};
    supported_config.concurrent_samples_count = 1;
    supported_config.async_processing = false;
    supported_config.samples_time_sync_mode = video_module_interface::supported_module_config::time_sync_mode::time_synced_input_only;

    video_module_interface::supported_image_stream_config & depth_desc = supported_config[stream_type::depth];
    depth_desc.size.width = 640;
    depth_desc.size.height = 480;
    depth_desc.frame_rate = 30;
    depth_desc.is_enabled = true;
    video_module_interface::supported_image_stream_config & color_desc = supported_config[stream_type::color];
    color_desc.size.width = 640;
    color_desc.size.height = 480;
    color_desc.frame_rate = 30;
    color_desc.is_enabled = true;

    m_module->set_custom_configs(supported_config);
    m_pipeline->add_cv_module(m_module.get());
    m_pipeline->set_config(supported_config);
    m_pipeline->start(m_callback_handler.get());
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    ASSERT_TRUE(m_callback_handler->was_a_new_valid_sample_dispatched());
    ASSERT_TRUE(m_callback_handler->was_a_new_max_depth_value_dispatched()) <<"new valid cv module output wasn't dispatched, MIGHT FAIL IF SYNCING LOTS OF SAMPLES";
    m_pipeline->stop();
}

TEST_F(pipeline_tests, check_graceful_pipeline_destruction_while_streaming)
{
    m_pipeline->add_cv_module(m_module.get());
    m_pipeline->start(m_callback_handler.get());
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

TEST_F(pipeline_tests, check_pipeline_is_preventing_config_change_while_streaming)
{
    m_pipeline->add_cv_module(m_module.get());
    video_module_interface::supported_module_config available_config = {};
    m_pipeline->query_default_config(0, available_config);
    m_pipeline->set_config(available_config);
    EXPECT_EQ(status_invalid_state, m_pipeline->add_cv_module(m_module.get())) << "the pipeline should not allow adding cv module while streaming";
    m_pipeline->start(m_callback_handler.get());
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_EQ(status_invalid_state, m_pipeline->add_cv_module(m_module.get())) << "the pipeline should not allow adding cv module while streaming";
    m_pipeline->stop();
}

TEST_F(pipeline_tests, check_pipeline_recording_playing_a_recorded_file)
{
    const char * test_file = "pipeline_test.rssdk";
    auto is_file_exists = [](const std::string& file) { ifstream f(file.c_str()); return f.is_open(); };
    if(is_file_exists(test_file))
    {
        std::remove(test_file);
    }

    ASSERT_FALSE(is_file_exists(test_file));

    m_pipeline.reset(new pipeline_async(pipeline_async::testing_mode::record, test_file));

    m_pipeline->add_cv_module(m_module.get());
    ASSERT_EQ(status_no_error, m_pipeline->start(m_callback_handler.get()));
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_TRUE(m_callback_handler->was_a_new_valid_sample_dispatched()) <<"new valid sample wasn't dispatched";
    ASSERT_EQ(status_no_error, m_pipeline->stop());
    m_pipeline.reset();

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    m_module.reset(new max_depth_value_module_testing());
    m_callback_handler.reset(new pipeline_handler(m_module));
    m_pipeline.reset(new pipeline_async(pipeline_async::testing_mode::playback, test_file));
    m_pipeline->add_cv_module(m_module.get());
    ASSERT_EQ(status_no_error, m_pipeline->start(m_callback_handler.get()));
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_TRUE(m_callback_handler->was_a_new_valid_sample_dispatched()) <<"new valid sample wasn't dispatched";
    ASSERT_EQ(status_no_error, m_pipeline->stop());

    if(is_file_exists(test_file))
    {
        std::remove(test_file);
    }
}

GTEST_TEST(config_util_test, check_generete_matching_supersets)
{
    std::vector<std::vector<video_module_interface::supported_module_config>> groups;
    video_module_interface::supported_module_config config1 = {};
    video_module_interface::supported_module_config config2 = {};
    video_module_interface::supported_module_config config3 = {};
    std::vector<video_module_interface::supported_module_config> matching_supersets;

    auto clear = [&]()
    {
        groups.clear();
        config1 = {};
        config2 = {};
        config3 = {};
        matching_supersets.clear();
    };

    const char * device_name = "Temp";
    std::memcpy(config2.device_name, device_name, std::strlen(device_name));
    groups = {{config1}, {config2}, {config3}};
    config_util::generete_matching_supersets(groups, matching_supersets);
    ASSERT_TRUE(matching_supersets.size() == 1 && std::strcmp(matching_supersets.at(0).device_name, device_name) == 0) << "can get matched device name";

    //check configs match by device name
    clear();
    std::memcpy(config2.device_name, device_name, std::strlen(device_name));
    std::memcpy(config3.device_name, device_name, std::strlen(device_name));
    groups = {{config1}, {config2}, {config3}};
    config_util::generete_matching_supersets(groups, matching_supersets);
    ASSERT_TRUE(matching_supersets.size() == 1 && std::strcmp(matching_supersets.at(0).device_name, device_name) == 0);

    //check conflicted configs by the device name are filtered out
    clear();
    std::memcpy(config2.device_name, device_name, std::strlen(device_name));
    const char * conflicted_device_name = "Conflict";
    std::memcpy(config3.device_name, conflicted_device_name , std::strlen(conflicted_device_name));
    groups = {{config1}, {config2}, {config3}};
    config_util::generete_matching_supersets(groups, matching_supersets);
    ASSERT_TRUE(matching_supersets.size() == 0);

    //check basic flattening of configs
    clear();
    config2[stream_type::color].size = {640, 0};
    config2[stream_type::color].is_enabled = true;
    groups = {{config1}, {config2}, {config3}};
    config_util::generete_matching_supersets(groups, matching_supersets);
    ASSERT_TRUE(matching_supersets.size() == 1 && matching_supersets.at(0)[stream_type::color].size.width == 640);

    //check conflicted configs by resolution are filtered out
    clear();
    config2[stream_type::color].size = {640, 0};
    config2[stream_type::color].is_enabled = true;
    config3[stream_type::color].size = {1280, 0};
    config3[stream_type::color].is_enabled = true;
    groups = {{config1}, {config2}, {config3}};
    config_util::generete_matching_supersets(groups, matching_supersets);
    ASSERT_TRUE(matching_supersets.size() == 0);

    //check that configs with different enabled streams are flattened
    clear();
    config2[stream_type::color].size = {640, 0};
    config2[stream_type::color].is_enabled = true;
    config3[stream_type::depth].size = {1280, 0};
    config3[stream_type::depth].is_enabled = true;
    groups = {{config1}, {config2}, {config3}};
    config_util::generete_matching_supersets(groups, matching_supersets);
    ASSERT_TRUE(matching_supersets.size() == 1 && matching_supersets.at(0)[stream_type::color].size.width == 640 &&
                                                  matching_supersets.at(0)[stream_type::depth].size.width == 1280);

    //check supersets are generated for config groups
    clear();
    config2[stream_type::color].size = {640, 0};
    config2[stream_type::color].is_enabled = true;
    groups = {{config2, config1}, {config2}, {config1, config3}};
    config_util::generete_matching_supersets(groups, matching_supersets);
    ASSERT_TRUE(matching_supersets.size() == 4 && (matching_supersets.at(0)[stream_type::color].size.width == 640)
                                               && (matching_supersets.at(1)[stream_type::color].size.width == 640)
                                               && (matching_supersets.at(2)[stream_type::color].size.width == 640)
                                               && (matching_supersets.at(3)[stream_type::color].size.width == 640));
}

