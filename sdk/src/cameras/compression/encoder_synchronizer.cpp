// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "encoder_synchronizer.h"
#include "dispatcher.h"

namespace rs
{
    namespace core
    {
        namespace compression
        {
            class encoding_thread : private rs::utils::dispatcher
            {
            public:
                encoding_thread(uint32_t buffer_size) : m_busy(false), m_disposed(false)
                {
                    m_encoded_data.resize(buffer_size);
                }

                virtual ~encoding_thread()
                {
                    std::unique_lock<std::mutex> locker(m_mutex);
                    m_disposed = true;
                    locker.unlock();

                    m_wait_handle.notify_one();
                    sync();
                }

                void encode_sample_and_lock(std::shared_ptr<encoder> encoder, std::shared_ptr<core::file_types::frame_sample> sample, rs::utils::async_action** pp_async_action)
                {
                    begin_invoke([this, encoder, sample]() -> void
                    {
                        std::unique_lock<std::mutex> locker(m_mutex);
                        m_wait_handle.wait(locker,  [this]() -> bool { return (m_disposed == true || m_busy == false); });

                        m_last_status = rs::core::status::status_handle_invalid;
                        if (m_disposed == true)
                            return;

                        m_busy = true;
                        locker.unlock();

                        uint32_t data_size = 0;
                        sample->finfo.ctype = encoder->get_compression_type(sample->finfo.stream);
                        if(sample->finfo.ctype != core::file_types::compression_type::none)
                        {
                            m_last_status = encoder->encode_frame(sample->finfo, sample->data, m_encoded_data.data(), data_size);
                            if (m_last_status == rs::core::status::status_no_error)
                            {
                                m_data_size = data_size;
                            }
                        }

                    }, pp_async_action);
                }

                uint8_t* data(uint32_t& data_size)
                {
                    if (m_last_status != rs::core::status::status_no_error)
                    {
                        data_size = 0;
                        return nullptr;
                    }

                    data_size = m_data_size;
                    return m_encoded_data.data();
                }

                void release_locked_sample()
                {
                    std::unique_lock<std::mutex> locker(m_mutex);
                    m_busy = false;
                    locker.unlock();

                    m_wait_handle.notify_one();
                }

            private:
                std::condition_variable m_wait_handle;
                std::vector<uint8_t> m_encoded_data;
                uint32_t m_data_size;
                rs::core::status m_last_status;
                bool m_busy;
                bool m_disposed;
                std::mutex m_mutex;
            };

            class stream_encoder
            {
            public:
                stream_encoder(uint32_t buffer_size, uint32_t concurrency) : m_next_thread_index(0)
                {
                    for (uint32_t i = 0; i < concurrency; i++)
                    {
                        m_threads.push_back( std::make_shared<encoding_thread>(buffer_size));
                    }
                }

                ~stream_encoder()
                {
                }

                void encode_sample_and_lock(std::shared_ptr<encoder> encoder, std::shared_ptr<core::file_types::frame_sample> sample)
                {
                    std::lock_guard<std::mutex> locker(m_mutex);

                    size_t index = m_next_thread_index;
                    m_next_thread_index = ((m_next_thread_index +1) % m_threads.size());
                    rs::utils::async_action* p_async_action = nullptr;
                    m_threads[index]->encode_sample_and_lock(encoder, sample, &p_async_action);
                    m_actions.push({
                                       std::shared_ptr<rs::utils::async_action>(p_async_action, [](rs::utils::async_action* p_action) { p_action->release();}),
                                       index
                                   });
                }

                uint8_t* get_next_data(uint32_t& data_size)
                {
                    std::unique_lock<std::mutex> locker(m_mutex);

                    if(m_actions.size() == 0)
                        return nullptr;

                    std::shared_ptr<rs::utils::async_action> action = m_actions.front().first;

                    if(action == nullptr)
                        return nullptr;

                    size_t index = m_actions.front().second;
                    locker.unlock();

                    action->wait();
                    rs::utils::async_state state = action->state();
                    if (state != rs::utils::async_state::completed)
                    {
                        data_size = 0;
                        return nullptr;
                    }

                    return m_threads[index]->data(data_size);
                }

                void release_locked_sample()
                {
                    std::unique_lock<std::mutex> locker(m_mutex);
                    size_t index = m_actions.front().second;
                    m_actions.pop();

                    m_threads[index]->release_locked_sample();
                }

            private:
                std::vector<std::shared_ptr<encoding_thread>> m_threads;
                size_t m_next_thread_index;
                std::queue<std::pair<std::shared_ptr<rs::utils::async_action>, size_t>> m_actions;
                std:: mutex m_mutex;
            };

            encoder_synchronizer::encoder_synchronizer(std::shared_ptr<compression::encoder> encoder) : m_encoder(encoder)
            {

            }

            encoder_synchronizer::~encoder_synchronizer()
            {
                clear();
            }

            void encoder_synchronizer::add_stream(rs_stream stream, uint32_t buffer_size, uint32_t concurrency)
            {
                std::unique_lock<std::mutex> locker(m_mutex);
                m_encoders[stream] = std::make_shared<stream_encoder>(buffer_size, concurrency);
            }

            void encoder_synchronizer::clear()
            {
                std::lock_guard<std::mutex> locker(m_mutex);
                m_encoders.clear();
            }

            void encoder_synchronizer::encode_sample_and_lock(std::shared_ptr<core::file_types::frame_sample> sample)
            {
                m_encoders[sample->finfo.stream]->encode_sample_and_lock(m_encoder, sample);
            }

            uint8_t* encoder_synchronizer::get_next_data(rs_stream stream, uint32_t& data_size)
            {
                return m_encoders[stream]->get_next_data(data_size);
            }

            void encoder_synchronizer::release_locked_sample(rs_stream stream)
            {
                m_encoders[stream]->release_locked_sample();
            }

            file_types::compression_type encoder_synchronizer::get_compression_type(rs_stream stream)
            {
                return m_encoder->get_compression_type(stream);
            }
        }
    }
}
