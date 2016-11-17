// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <limits.h>
#include <map>
#include "gtest/gtest.h"
#include "utilities/utilities.h"
#include <thread>

//librealsense api
#include "librealsense/rs.hpp"

#include "rs_sdk.h"

using namespace rs::core;
class samples_sync_tests : public testing::Test
{
protected:
    std::unique_ptr<rs::core::context> m_context;
    rs::device * m_device;

    static int m_frames_sent;
    static int m_sets_received;
    static int m_max_fps;
    static int m_total_frames_sent;
    static int m_total_frames_received;
    static int m_max_unmatched_frames;


    static void TearDownTestCase()
    {
    }

    void CleanAllUnmatched(rs::utils::unique_ptr<rs::utils::samples_time_sync_interface>& samples_sync)
    {
        for (int i =0; i<static_cast<int>(rs::core::stream_type::max); i++)
        {
            stream_type str_type = static_cast<stream_type>(i);
            image_interface* ptr = nullptr;
            bool more=true;
            while (more)
            {
                more = samples_sync->get_not_matched_frame(str_type, &ptr);
                if (ptr)
                {
                    m_total_frames_received++;
                    ptr->release();
                    ptr = nullptr;
                }
            }
        }
    }

    static void CheckCorrelatedSet(correlated_sample_set& correlated_sample)
    {
        m_sets_received++;
        double timestamp=0;
        int ts_fps;

        for (int i =0; i<static_cast<int>(rs::core::stream_type::max); i++)
        {
            stream_type str_type = static_cast<stream_type>(i);

            if (correlated_sample[str_type] && str_type!= stream_type::fisheye)
            {
                m_total_frames_received++;
                if (timestamp==0) {
                    timestamp = correlated_sample[str_type]->query_time_stamp();

                }
                ASSERT_EQ(timestamp, correlated_sample[str_type]->query_time_stamp()) << "Error - timestamp not equal";
            }
        }


        if (correlated_sample[stream_type::fisheye])
        {
            m_total_frames_received++;
            double period = 1000.0 / m_max_fps /2;
            auto diff= correlated_sample[stream_type::fisheye]->query_time_stamp() - timestamp;
            //just in case we have just fisheye
            if (!timestamp) diff=0;
            diff= std::abs(diff);
            ASSERT_LE(diff, period) << "Fisheye timestamp is not in interval";
        }

     }

    virtual void SetUp()
    {
        //create a record enabled context with a given output file
        m_context = std::unique_ptr<rs::core::context>(new rs::core::context());

        ASSERT_NE(0, m_context->get_device_count()) << "no device detected";

        //each device created from the record enabled context will write the streaming data to the given file
        m_device = m_context->get_device(0);

        ASSERT_NE(nullptr, m_device);
        m_frames_sent = 0;
        m_sets_received = 0;
        m_max_fps=0;
        m_total_frames_sent = m_total_frames_received = 0;
        m_max_unmatched_frames = 0;

    }

    void Setup_and_run(int streams[static_cast<int>(rs::core::stream_type::max)], int motions[static_cast<int>(rs::core::motion_type::max)], bool check_not_full = false)
    {
        const rs::format color_format = rs::format::rgb8;
        const rs::format depth_format = rs::format::z16;
        const rs::format ir_format = rs::format::y16;

        auto run_time = 5;

        int latency=100;
        int buffer_size=0;

        if (check_not_full)
            buffer_size=3;

        // create samples_time_sync object
        rs::utils::unique_ptr<rs::utils::samples_time_sync_interface> samples_sync = rs::utils::get_unique_ptr_with_releaser(
                                            rs::utils::samples_time_sync_interface::create_instance(streams, motions, m_device->get_name(), latency, buffer_size));

        bool keep_accepting = true;

        int lowest_fps = 1000;
        stream_type slowest_stream;

        auto callback = [&samples_sync, this, &keep_accepting, &streams, &slowest_stream](rs::frame new_frame)
        {
            using namespace rs::core;
            using namespace rs::utils;
            rs::core::correlated_sample_set correlated_sample = {};

            if (!keep_accepting)
                return;

            /*std::cout << "Received video frame " << new_frame.get_frame_number() << " , type " << static_cast<int>(new_frame.get_stream_type()) << " with timestamp " << new_frame.get_timestamp()
                      << " TS Domain: " << static_cast<int>(new_frame.get_frame_timestamp_domain()) << std::endl;*/

            auto  image = get_unique_ptr_with_releaser(image_interface::create_instance_from_librealsense_frame(new_frame,
                                                                               image_interface::flag::any));

            m_total_frames_sent++;

            auto st = samples_sync->insert(image.get(), correlated_sample);

            if (image->query_stream_type() == slowest_stream)
                m_frames_sent++;

            if (st)
            {
                CheckCorrelatedSet(correlated_sample);

                for(int i = 0; i < static_cast<uint8_t>(stream_type::max); i++)
                {
                    if(correlated_sample.images[i])
                    {
                        correlated_sample.images[i]->release();
                        correlated_sample.images[i] = nullptr;
                    }
                }
            }
            CleanAllUnmatched(samples_sync);


        };


        for (int i =0; i<static_cast<int>(rs::core::stream_type::max); i++)
        {
            if (streams[i] != 0)
            {
                m_max_unmatched_frames += streams[i]/10;

                m_max_fps = std::max(m_max_fps, streams[i]);
                if (streams[i]<lowest_fps) {
                    lowest_fps=streams[i];
                    slowest_stream=static_cast<stream_type>(i);
                }

                int fps = streams[i];
                int w, h;
                rs::format format = color_format;
                rs::core::stream_type st = static_cast<rs::core::stream_type>(i);
                if (st == stream_type::color) { w=640; h=480; format = color_format;}
                else { w=628; h=468; format = depth_format;}
                if (st==stream_type::infrared || st==stream_type::infrared2)
                    { format = ir_format; }
                if (st==stream_type::fisheye) {
                    format = rs::format::raw8; w=640; h=480;
                }

                m_device->enable_stream(rs::utils::convert_stream_type(st),w,h,format, fps);
                m_device->set_frame_callback(rs::utils::convert_stream_type(st), callback);
            }
        }

        // add strobe for fisheye - in order to get correlated samples with depth stream
        if (streams[static_cast<int>(stream_type::fisheye)] != 0)
            m_device->set_option(rs::option::fisheye_strobe, 1);

        auto motion_callback = [&keep_accepting, &samples_sync](rs::motion_data data)
        {
            using namespace rs::core;
            using namespace rs::utils;
            rs::core::correlated_sample_set correlated_sample;

            if (!keep_accepting)
                return;

            motion_sample new_sample;

            // create a motion sample from data
            int i=0;
            while (i<3)
            {
                new_sample.data[i] = data.axes[i];
                i++;
            }

            new_sample.timestamp = data.timestamp_data.timestamp;
            new_sample.type = (motion_type)(data.timestamp_data.source_id == RS_EVENT_IMU_ACCEL ? 1 : 2);

            auto st = samples_sync->insert(new_sample, correlated_sample);

            if (st)
            {
                CheckCorrelatedSet(correlated_sample);
            }


        };

        for (int i =0; i<static_cast<int>(rs::core::motion_type::max); i++)
        {
            // enable motion in device
            if (motions[i])
                m_device->enable_motion_tracking(motion_callback);
        }

        m_device->start(rs::source::all_sources);
        std::this_thread::sleep_for(std::chrono::seconds(run_time));

        CleanAllUnmatched(samples_sync);
        keep_accepting = false;
        samples_sync->flush();
        m_device->stop(rs::source::all_sources);

        auto diff = m_frames_sent-m_sets_received;

        diff = std::abs(diff);

        ASSERT_EQ(diff<=10, true) << "Sent: " << m_frames_sent << " Received: " << m_sets_received;
        diff = m_total_frames_sent-m_total_frames_received;

        if (check_not_full)
        {
            ASSERT_EQ(diff<=m_max_unmatched_frames, true) << "Sent: " << m_total_frames_sent << "Total Received: " <<  m_total_frames_received;
        }

    }
};




TEST_F(samples_sync_tests, basic_time_sync_test)
{

    int streams[static_cast<int>(rs::core::stream_type::max)] = {0};
    int motions[static_cast<int>(rs::core::motion_type::max)] = {0};

    // create arrays of fps valuse for streams and motions
    streams[static_cast<int>(rs::core::stream_type::color)] = 30;
    streams[static_cast<int>(rs::core::stream_type::depth)] = 30;


    Setup_and_run(streams, motions);

}

TEST_F(samples_sync_tests, basic_time_sync_test_2)
{

    int streams[static_cast<int>(rs::core::stream_type::max)] = {0};
    int motions[static_cast<int>(rs::core::motion_type::max)] = {0};

    motions[static_cast<int>(motion_type::accel)] = 200;
    motions[static_cast<int>(motion_type::gyro)] = 200;

    // create arrays of fps valuse for streams and motions
    streams[static_cast<int>(rs::core::stream_type::color)] = 60;
    streams[static_cast<int>(rs::core::stream_type::depth)] = 60;
    streams[static_cast<int>(rs::core::stream_type::fisheye)] = 30;

    Setup_and_run(streams, motions);

}

TEST_F(samples_sync_tests, time_sync_test_with_unmatched)
{

    int streams[static_cast<int>(rs::core::stream_type::max)] = {0};
    int motions[static_cast<int>(rs::core::motion_type::max)] = {0};

    motions[static_cast<int>(motion_type::accel)] = 200;
    motions[static_cast<int>(motion_type::gyro)] = 200;

    // create arrays of fps valuse for streams and motions
    streams[static_cast<int>(rs::core::stream_type::color)] = 60;
    streams[static_cast<int>(rs::core::stream_type::depth)] = 60;
    streams[static_cast<int>(rs::core::stream_type::fisheye)] = 30;

    Setup_and_run(streams, motions, true);

}

class samples_sync_external_camera_tests : public testing::Test
{
public:
    static std::shared_ptr<image_interface> create_dummy_image(rs::core::stream_type st, int num_frame)
    {
        image_info info = {};
        auto now = std::chrono::system_clock::now();
        double timestamp = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
        
        rs::core::image_interface::image_data_with_data_releaser image(nullptr, nullptr);
        return rs::utils::get_shared_ptr_with_releaser(image_interface::create_instance_from_raw_data(&info, image, st, image_interface::flag::any, timestamp, num_frame));
    }
};

class smart_correlated_sample_set
{
public:
    ~smart_correlated_sample_set()
    {
        for(int i=0; i< static_cast<uint8_t>(stream_type::max); i++)
        {
            if(m_sample_set[static_cast<stream_type>(i)] != nullptr)
            {
                m_sample_set[static_cast<stream_type>(i)]->release();
            }
        }
    }
    
    rs::core::correlated_sample_set& get()
    {
        return m_sample_set;
    }
private:
    rs::core::correlated_sample_set m_sample_set = {};
};
TEST_F(samples_sync_external_camera_tests, basic_sync)
{
    int streams[static_cast<int>(rs::core::stream_type::max)] = {0};
    int motions[static_cast<int>(rs::core::motion_type::max)] = {0};
    
    
    streams[static_cast<int>(rs::core::stream_type::color)] = 30;
    streams[static_cast<int>(rs::core::stream_type::depth)] = 30;
    
    rs::utils::unique_ptr<rs::utils::samples_time_sync_interface> samples_sync = rs::utils::get_unique_ptr_with_releaser(
        rs::utils::samples_time_sync_interface::create_instance(streams, motions, rs::utils::samples_time_sync_interface::external_device_name));
    
    {
        auto color_image = create_dummy_image(rs::core::stream_type::color, 1);
        auto depth_image = create_dummy_image(rs::core::stream_type::depth, 1);
        
        smart_correlated_sample_set sample_set;
        
        ASSERT_FALSE(samples_sync->insert(color_image.get(), sample_set.get()));
        ASSERT_TRUE(samples_sync->insert(depth_image.get(), sample_set.get()));
        ASSERT_EQ(sample_set.get()[stream_type::color]->query_frame_number(), 1u);
        ASSERT_EQ(sample_set.get()[stream_type::depth]->query_frame_number(), 1u);
        samples_sync->flush();
    }
    
    
    {
        auto color_image1 = create_dummy_image(rs::core::stream_type::color, 1);
        auto color_image2 = create_dummy_image(rs::core::stream_type::color, 2);
        auto depth_image = create_dummy_image(rs::core::stream_type::depth, 1);
        
        smart_correlated_sample_set sample_set;
        
        ASSERT_FALSE(samples_sync->insert(color_image1.get(), sample_set.get()));
        ASSERT_FALSE(samples_sync->insert(color_image2.get(), sample_set.get()));
        ASSERT_TRUE(samples_sync->insert(depth_image.get(), sample_set.get()));
        ASSERT_EQ(sample_set.get()[stream_type::color]->query_frame_number(), 2u);
        ASSERT_EQ(sample_set.get()[stream_type::depth]->query_frame_number(), 1u);
        samples_sync->flush();
    }
    
    {
        auto color_image1 = create_dummy_image(rs::core::stream_type::color, 5);
        auto depth_image1 = create_dummy_image(rs::core::stream_type::depth, 6);
        auto depth_image2 = create_dummy_image(rs::core::stream_type::depth, 7);
        
        smart_correlated_sample_set sample_set;
        
        ASSERT_FALSE(samples_sync->insert(depth_image1.get(), sample_set.get()));
        ASSERT_FALSE(samples_sync->insert(depth_image2.get(), sample_set.get()));
        ASSERT_TRUE(samples_sync->insert(color_image1.get(), sample_set.get()));
        ASSERT_EQ(sample_set.get()[stream_type::color]->query_frame_number(), 5u);
        ASSERT_EQ(sample_set.get()[stream_type::depth]->query_frame_number(), 7u);
        samples_sync->flush();
        
    }
    
}

int samples_sync_tests::m_frames_sent=0;
int samples_sync_tests::m_sets_received=0;
int samples_sync_tests::m_max_fps=0;
int samples_sync_tests::m_total_frames_sent=0;
int samples_sync_tests::m_total_frames_received=0;
int samples_sync_tests::m_max_unmatched_frames=0;
