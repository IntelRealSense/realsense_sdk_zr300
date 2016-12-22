// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <functional>
#include <condition_variable>
#include <stdexcept>
#include <mutex>
#include <chrono>
#include <queue>
#include <memory>
#include <thread>
#include <atomic>
#include <map>
#include <algorithm>
#include <atomic>
#include "rs/utils/ref_count_base.h"
#include "scope_guard.h"

namespace rs
{
    namespace utils
    {
        static const uint32_t ACTIONS_ALLOCATOR_RESERVE_SIZE = 64;
        static const uint32_t TIMERS_ALLOCATOR_RESERVE_SIZE = 32;

        enum async_state
        {
            pending,
            running,
            completed,
            exception,
            cancelled
        };

        using clock = std::chrono::high_resolution_clock;

        struct invokable_interface : public rs::core::ref_count_interface
        {
            virtual void invoke() = 0;
        };

        struct action_interface : public invokable_interface
        {
            virtual void cancel() = 0;
        };

        class context_interface
        {
        public:
            virtual unsigned long id() = 0;
            virtual bool disposed() = 0;
            virtual bool invoke_required() = 0;
            virtual bool idle() = 0;
        };

        class context_exception : public std::exception
        {
        private:
            context_interface& m_context;

        public:
            context_exception(context_interface& context) : m_context(context) {}
            context_interface& context()
            {
                return m_context;
            }
        };

        class exception_handler_interface
        {
        public:
            virtual void on_exception() = 0;
            virtual void on_exception(context_exception& e) = 0;
            virtual void on_exception(std::exception& e) = 0;
        };

        class async_action_sync_exception : public context_exception
        {
        public:
            async_action_sync_exception(context_interface& context) : context_exception(context) {}

            virtual const char* what() const noexcept override
            {
                return "AsyncAction cannot be synchronized from the context it's scheduled to run or currently running";
            }
        };

        class context_disposed_exception : public context_exception
        {
        public:
            context_disposed_exception(context_interface& context) : context_exception(context) {}

            virtual const char* what() const noexcept override
            {
                return "Context disposed";
            }
        };


        class base_async_action : public rs::utils::ref_count_base<action_interface>
        {
            friend class dispatcher;

        private:
            async_state m_state;
            bool m_completed_synchronously;
            std::mutex m_mutex;
            std::condition_variable m_wait_handle;
            context_interface& m_context;
            base_async_action(const base_async_action& other);			// non construction-copyable
            base_async_action(base_async_action&& other);					// non construction-movable
            base_async_action& operator=(const base_async_action&) = delete;		// non copyable
            base_async_action& operator=(base_async_action&& other) = delete;		// non movable

            void set_state(async_state state)
            {
                    std::lock_guard<std::mutex> locker(m_mutex);
                    m_state = state;
            }

            void signal(async_state state)
            {
                    set_state(state);
                    m_wait_handle.notify_all();
            }

            virtual void invoke() override
            {
                bool completed_successfully = false;

                rs::utils::scope_guard scopeGuard([this, &completed_successfully]() -> void
                {
                        signal((completed_successfully == true) ? async_state::completed : async_state::exception);
                });

                set_state(async_state::running);
                perform();
                completed_successfully = true;
            }

            virtual void cancel() override
            {
                signal(async_state::cancelled);
            }

        protected:
            base_async_action(context_interface& context, bool completedSynchronously) :
                m_state(async_state::pending),
                m_completed_synchronously(completedSynchronously),
                m_context(context) {}
                virtual void perform() = 0;
        public:
            async_state state()
            {
                    std::unique_lock<std::mutex> locker(m_mutex);
                    async_state retVal = m_state;
                    locker.unlock();
                    return retVal;
            }

            bool completed_synchronously()
            {
                    return m_completed_synchronously;
            }

            bool wait(long timeout = 0)
            {
                if (timeout < 0)
                    throw std::out_of_range("timeout");

                std::unique_lock<std::mutex> locker(m_mutex);

                if ((m_state == async_state::pending || m_state == async_state::running) && m_context.invoke_required() == false)
                {
                    throw async_action_sync_exception(m_context);
                }

                auto pred = [this]() -> bool
                {
                    return ((m_state == async_state::completed) || (m_state == async_state::exception) || (m_state == async_state::cancelled));
                };

                if (timeout == 0)
                {
                    m_wait_handle.wait(locker, pred);
                    return (m_state != async_state::cancelled);
                }
                else
                {
                    return ((m_wait_handle.wait_for(locker, std::chrono::milliseconds(timeout), pred) == true) && (m_state != async_state::cancelled));
                }
            }
        };

        class async_action : public base_async_action
        {
        private:
            std::function<void()> m_func;

        protected:
            virtual void perform() override
            {
                rs::utils::scope_guard releaser([this]() -> void
                {
                    m_func = nullptr;
                });

                m_func();
            }

        public:
            async_action(context_interface& context, const std::function<void()>& func, bool completed_synchronously) :
                base_async_action(context, completed_synchronously), m_func(func)
            {
            }
            virtual ~async_action() = default;
        };

        template <typename T>
        class async_result_action : public base_async_action
        {
        private:
            T m_result;
            std::function<T()> m_func;
            std::function<void(const T&)> m_result_callback;

        protected:
            virtual void perform() override
            {
                rs::utils::scope_guard releaser([this]() -> void
                {
                    m_func = nullptr;
                    m_result_callback = nullptr;
                });

                m_result = m_func();

                if (m_result_callback != nullptr)
                {
                    m_result_callback(m_result);
                }
            }

        public:
            async_result_action(
                context_interface& context,
                const std::function<T()>& func,
                bool completed_synchronously,
                const std::function<void(const T&)>& result_callback) :
                base_async_action(context, completed_synchronously),
                    m_func(func), m_result_callback(result_callback) {}

                const T& result()
                {
                    wait();
                    return m_result;
                }
        };

        class dispatcher : public context_interface
        {
        private:
            class timer
            {
            private:
                static const uint32_t RESERVED_TIMER_CLIENTS_SIZE = 2;

                class client_wrapper
                {
                private:
                    static const int UNDEFINED_CLINET_ID = -1;
                    int m_id;
                    std::shared_ptr<invokable_interface> m_timer_client;
                    unsigned int m_invocation_count;
                    bool m_expired;

                    int get_next_client_id()
                    {
                        static int NEXT_ID = 0;
                        static std::mutex ID_MUTEX;
                        std::lock_guard<std::mutex> locker(ID_MUTEX);
                        int retval = NEXT_ID++;
                        if (NEXT_ID == (std::numeric_limits<int>::max)())
                            NEXT_ID = 0;
                        return retval;
                    }

                public:
                    client_wrapper() : m_id(UNDEFINED_CLINET_ID), m_invocation_count(0)
                    {
                    }
                    client_wrapper(invokable_interface* timer_client, unsigned int invocation_count) :
                        m_id(get_next_client_id()),
                        m_invocation_count(invocation_count),
                        m_expired(false)
                    {
                        timer_client->add_ref();
                        m_timer_client = std::shared_ptr<invokable_interface>(timer_client, [](invokable_interface* client) -> void
                        {
                            client->release();
                        });
                    }
                    int id()
                    {
                        return m_id;
                    }
                    bool expired()
                    {
                        return m_expired;
                    }
                    void invoke()
                    {
                        if (m_invocation_count > 0)
                        {
                            --m_invocation_count;
                            if (m_invocation_count == 0)
                            {
                                m_expired = true;
                            }
                        }
                        m_timer_client->invoke();
                    }
                };

            double m_interval;
            std::mutex m_mutex;
            std::vector<std::shared_ptr<client_wrapper>> m_clients;
            std::vector<std::shared_ptr<client_wrapper>> m_executing_clients;

            std::vector<std::shared_ptr<client_wrapper>>::iterator find_client(int id)
            {
                for (auto it = m_clients.begin(); it != m_clients.end(); it++)
                {
                    if ((*it)->id() == id)
                    {
                        return it;
                    }
                }
                return m_clients.end();
            }

            public:
                timer(double interval) : m_interval(interval)
                {
                    m_clients.reserve(RESERVED_TIMER_CLIENTS_SIZE);
                    m_executing_clients.reserve(RESERVED_TIMER_CLIENTS_SIZE);
                }

                double interval()
                {
                    return m_interval;
                }

                int client_count()
                {
                    std::unique_lock<std::mutex> locker(m_mutex);
                    int retval = static_cast<int>(m_clients.size());
                    return retval;
                }

                int add_client(invokable_interface* timer_client, unsigned int invocation_count)
                {
                    std::lock_guard<std::mutex> locker(m_mutex);
                    std::shared_ptr<client_wrapper> client = std::make_shared<client_wrapper>(timer_client, invocation_count);
                    m_clients.emplace_back(client);
                    return client->id();
                }

                bool remove_client(int id)
                {
                    bool retval = false;
                    std::lock_guard<std::mutex> locker(m_mutex);
                    auto it = find_client(id);
                    if (it != m_clients.end())
                    {
                        m_clients.erase(it);
                        retval = true;
                    }
                    return retval;
                }

                bool is_expired(clock::time_point& now, clock::time_point& reference_time, unsigned long long& expire_interval)
                {
                    bool retval = false;
                    unsigned long long time_interval = ((std::chrono::duration_cast<std::chrono::microseconds>)(now - reference_time)).count();

                    if (time_interval >= static_cast<unsigned long long>(m_interval * 1000.0))
                    {
                        expire_interval = 0;
                        retval = true;
                    }
                    else
                    {
                        expire_interval = static_cast<unsigned long long>(m_interval * 1000.0) - time_interval;
                    }
                    return retval;
                }

                void invoke()
                {
                    std::unique_lock<std::mutex> locker(m_mutex);
                    m_executing_clients.insert(m_executing_clients.end(), m_clients.begin(), m_clients.end());
                    locker.unlock();

                    rs::utils::scope_guard executing_clients_clear([this]() -> void
                    {
                        m_executing_clients.clear();
                    });

                    for (auto& client : m_executing_clients)
                    {
                        client->invoke();
                        if (client->expired())
                            remove_client(client->id());
                    }
                }
            };

            class timer_client : public rs::utils::ref_count_base<invokable_interface>
            {
            private:
                context_interface& m_context;
                std::function<void()> m_func;

            public:
                timer_client(context_interface& context, const std::function<void()>& func) :
                    m_context(context), m_func(func) {}

                virtual void invoke() override
                {
                    m_func();
                }
            };

            std::thread m_invocation_thread;
            unsigned long m_id;
            std::atomic<bool> m_running;
            bool* m_stack_stopper;
            bool m_idle;
            bool m_adding_timer;
            exception_handler_interface* m_exception_handler;
            std::mutex m_mutex;
            std::condition_variable m_wait_handle;
            std::vector<action_interface*> m_actions;
            std::vector<std::pair<clock::time_point, std::shared_ptr<timer>>> m_timers;
            dispatcher(const dispatcher& other);                    // non construction-copyable
            dispatcher& operator=(const dispatcher&) = delete;		// non copyable

            unsigned long get_next_task_id()
            {
                static std::atomic<unsigned long> ID;
                return ID++;
            }

            void invoke(invokable_interface* invokable)
            {
                try
                {
                    invokable->invoke();
                }
                catch (context_exception& e)
                {
                    if (m_exception_handler != nullptr)
                    {
                        m_exception_handler->on_exception(e);
                    }
                    else
                    {
                        throw e;
                    }
                }
                catch (std::exception& e)
                {
                    if (m_exception_handler != nullptr)
                    {
                        m_exception_handler->on_exception(e);
                    }
                    else
                    {
                        throw e;
                    }
                }
                catch (...)
                {
                    if (m_exception_handler != nullptr)
                    {
                        m_exception_handler->on_exception();
                    }
                    else
                    {
                        throw;
                    }
                }
            }

            void worker()
            {
                bool stack_stopper = false;
                m_stack_stopper = &stack_stopper;
                std::vector<action_interface*> pending_actions;
                pending_actions.reserve(ACTIONS_ALLOCATOR_RESERVE_SIZE);
                std::vector<std::shared_ptr<timer>> pending_timers;
                pending_timers.reserve(TIMERS_ALLOCATOR_RESERVE_SIZE);
                clock::time_point deadline;
                unsigned long long wait_interval = 0;
                bool timer_iteration;

                auto pred = [this]() -> bool
                {
                    return ((m_actions.size() > 0) || (m_adding_timer == true) || (m_running == false));
                };

                while (stack_stopper == false)
                {
                    pending_timers.clear();
                    std::unique_lock<std::mutex> locker(m_mutex);
                    m_idle = true;
                    if (get_next_timers(pending_timers, deadline, wait_interval) == false)
                    {
                        timer_iteration = false;
                        m_wait_handle.wait(locker, pred);
                    }
                    else
                    {
                        if (wait_interval > 0)
                        {
                            timer_iteration = (m_wait_handle.wait_until(locker, deadline, pred) == false);
                        }
                        else
                        {
                            timer_iteration = true;
                        }
                    }

                    m_adding_timer = false;
                    std::swap(m_actions, pending_actions);

                    if (timer_iteration == true)
                        m_idle = (check_timers(deadline, pending_timers) == false);

                    if (m_idle == true)
                        m_idle = (pending_actions.size() == 0);

                    bool stop_thread = (m_running == false);
                    locker.unlock();

                    if (timer_iteration == true)
                    {
                        for (auto timer : pending_timers)
                        {
                            if (stack_stopper == true)
                                break;
                            timer->invoke();
                        }
                    }

                    for (auto action : pending_actions)
                    {
                        rs::utils::scope_guard action_releaser([&action]() -> void
                        {
                            action->release();
                        });
                        if (stack_stopper == true)
                        {
                            action->cancel();
                        }
                        else
                        {
                            invoke(action);
                        }
                    }

                    pending_actions.clear();

                    if (stop_thread == true)
                        break;
                }
            }

            bool get_next_timers(std::vector<std::shared_ptr<timer>>& timers, clock::time_point& deadline, unsigned long long& wait_interval)
            {
                // Clean empty timers
                m_timers.erase(std::remove_if(m_timers.begin(), m_timers.end(), [](const std::pair<clock::time_point, std::shared_ptr<timer>>& timePair) -> bool
                {
                    return (timePair.second->client_count() == 0);
                }), m_timers.end());

                wait_interval = (std::numeric_limits<unsigned long long>::max)();

                if (m_timers.size() > 0)
                {
                    clock::time_point now = clock::now();
                    for (auto it = m_timers.begin(); it != m_timers.end(); it++)
                    {
                        std::shared_ptr<dispatcher::timer> timer = it->second;
                        if (timer->client_count() > 0)
                        {
                            unsigned long long expire_interval;
                            timer->is_expired(now, it->first, expire_interval);
                            bool add_timer = false;
                            if (expire_interval < wait_interval)
                            {
                                timers.clear();
                                wait_interval = expire_interval;
                                deadline = now + std::chrono::microseconds(wait_interval);
                                add_timer = true;
                            }
                            else if (expire_interval == wait_interval)
                            {
                                add_timer = true;
                            }
                            if (add_timer == true)
                            {
                                timers.emplace_back(timer);
                            }
                        }
                    }
                }
                return (timers.size() > 0);
            }

            bool check_timers(clock::time_point& now, std::vector<std::shared_ptr<timer>>& timers)
            {
                std::vector<std::vector<std::shared_ptr<timer>>::iterator> removed_timers;
                for (std::vector<std::shared_ptr<timer>>::iterator itX = timers.begin(); itX != timers.end(); itX++)
                {
                    bool found = false;
                    for (auto itY = m_timers.begin(); itY != m_timers.end(); itY++)
                    {
                        if ((*itX) == itY->second)
                        {
                            std::pair<clock::time_point, std::shared_ptr<timer>> swap_pair(now, itY->second);
                            (*itY).swap(swap_pair);
                            found = true;
                            break;
                        }
                    }
                    if (found == false)
                        removed_timers.emplace_back(itX);
                }

                for (auto removedTimer : removed_timers)
                {
                    timers.erase(removedTimer);
                }
                return (timers.size() > 0);
            }

            void add_action(base_async_action* action)
            {
                std::unique_lock<std::mutex> locker(m_mutex);

                if (m_running == false)
                    throw context_disposed_exception(*(this));

                action->add_ref();
                m_actions.emplace_back(action);
                m_idle = false;
                locker.unlock();
                m_wait_handle.notify_one();
            }

        public:
            dispatcher(exception_handler_interface* exception_handler = nullptr):
                m_id(get_next_task_id()), m_running(true), m_adding_timer(false), m_exception_handler(exception_handler)
            {
                m_actions.reserve(ACTIONS_ALLOCATOR_RESERVE_SIZE);
                m_timers.reserve(TIMERS_ALLOCATOR_RESERVE_SIZE);
                m_invocation_thread = std::thread([this] { worker(); });
            }

            dispatcher(dispatcher&& other) : m_running(true)
            {
                std::lock_guard<std::mutex> locker(other.m_mutex);
                m_id = other.m_id;
                m_exception_handler = other.m_exception_handler;
                m_actions = std::move(other.m_actions);
                m_timers = std::move(other.m_timers);
                m_invocation_thread = std::thread([this] { worker(); });
            }

            virtual ~dispatcher()
            {
                bool expected = true;
                bool desired = false;
                if (m_running.compare_exchange_strong(expected, desired) == true)
                {
                    if (invoke_required() == true)
                    {
                        if (m_invocation_thread.joinable() == true)
                        {
                            m_wait_handle.notify_one();
                            m_invocation_thread.join();
                        }
                    }
                    else
                    {
                        // That's a bit tricky...
                        // Obviously we can't join a thread from its context...
                        // We use a stack variable belonging to the context to notify the invocation thread NOT to access any of the class members and stop gracefully.
                        *m_stack_stopper = true;
                        m_invocation_thread.detach();
                    }
                    // We need to clean (cancel) pending actions to avoid deadlocks of waiting executions
                    // We know for sure that no more action will be added since (m_running == false)
                    std::lock_guard<std::mutex> locker(m_mutex);
                    for (auto action : m_actions)
                    {
                        rs::utils::scope_guard action_releaser([&action]() -> void
                        {
                            action->release();
                        });
                        action->cancel();
                    }
                }
            }

            virtual unsigned long id() override
            {
                return m_id;
            }

            virtual bool disposed() override
            {
                return (m_running == false);
            }

            virtual bool invoke_required() override
            {
                return (std::this_thread::get_id() != m_invocation_thread.get_id());
            }

            virtual bool idle() override
            {
                std::lock_guard<std::mutex> locker(m_mutex);
                bool retval = m_idle;
                return retval;
            }

            void begin_invoke(const std::function<void()>& func, async_action** pp_async_action = nullptr, bool force_async = false)
            {
                base_async_action* action = nullptr;
                rs::utils::scope_guard releaser([&action]() -> void
                {
                    action->release();
                });

                if (force_async == true || invoke_required() == true)
                {
                    action = new async_action(*(this), func, false);
                    add_action(action);
                }
                else
                {
                    action = new async_action(*(this), func, true);
                    invoke(action);
                }

                if (pp_async_action != nullptr)
                {
                    action->add_ref();
                    *pp_async_action = dynamic_cast<async_action*>(action);
                }
            }

            template <typename T>
            void begin_invoke(const std::function<T()>& func, std::function<void(const T&)> result_callback = nullptr, async_result_action<T>** pp_async_result_action = nullptr, bool force_async = false)
            {
                base_async_action* action = nullptr;

                rs::utils::scope_guard releaser([&action]() -> void
                {
                    action->release();
                });

                if (force_async == true || invoke_required() == true)
                {
                    action = new async_result_action<T>(*(this), func, false, result_callback);
                    add_action(action);
                }
                else
                {
                    action = new async_result_action<T>(*(this), func, true, result_callback);
                    invoke(action);
                }

                if (pp_async_result_action != nullptr)
                {
                    action->add_ref();
                    *pp_async_result_action = dynamic_cast<async_result_action<T>*>(action);
                }
            }

            void end_invoke(async_action* p_action, async_state* p_action_state = nullptr)
            {
                if (p_action == nullptr)
                    throw std::invalid_argument("p_action");
                p_action->wait();
                if (p_action_state != nullptr)
                    *p_action_state = p_action->state();
            }

            template <typename T>
            T end_invoke(async_result_action<T>* p_action, async_state* p_action_state = nullptr)
            {
                if (p_action == nullptr)
                    throw std::invalid_argument("p_action");

                p_action->wait();
                if (p_action_state != nullptr)
                    *p_action_state = p_action->state();
                return p_action->result();
            }

            void invoke(const std::function<void()>& func, async_state* p_action_state = nullptr)
            {
                async_action* p_action = nullptr;
                begin_invoke(func, &p_action);

                rs::utils::scope_guard releaser([&p_action]() -> void
                {
                    p_action->release();
                });
                end_invoke(p_action, p_action_state);
            }

            template <typename T>
            T invoke(const std::function<T()>& func, async_state* p_action_state = nullptr)
            {
                async_result_action<T>* p_action = nullptr;
                begin_invoke<T>(func, nullptr, &p_action);

                rs::utils::scope_guard releaser([&p_action]() -> void
                {
                    p_action->release();
                });

                return end_invoke<T>(p_action, p_action_state);
            }

            void sync()
            {
                invoke([]() -> void {});
            }


            int register_timer(double interval, const std::function<void()>& func, unsigned int invocation_count = 0)
            {
                if (interval <= 0)
                    throw std::invalid_argument("interval");
                timer_client* p_timer_client = new timer_client(*(this), func);

                rs::utils::scope_guard timer_client_releaser([&p_timer_client]() -> void
                {
                    p_timer_client->release();
                });

                std::unique_lock<std::mutex> locker(m_mutex);
                auto it = std::find_if(m_timers.begin(), m_timers.end(), [&interval](const std::pair<clock::time_point, std::shared_ptr<timer>>& timePair) -> bool
                {
                    return (timePair.second->interval() == interval);
                });

                if (it != m_timers.end() && it->second->client_count() == 0)
                {
                    m_timers.erase(it);
                    it = m_timers.end();
                }

                std::shared_ptr<dispatcher::timer> timer;
                if (it != m_timers.end())
                {
                    timer = it->second;
                }
                else
                {
                    timer = std::make_shared<dispatcher::timer>(interval);
                    m_timers.emplace_back(std::pair<clock::time_point, std::shared_ptr<dispatcher::timer>>(clock::now(), timer));
                }

                int retval = timer->add_client(p_timer_client, invocation_count);
                m_adding_timer = true;
                locker.unlock();
                m_wait_handle.notify_one();
                return retval;
            }

            bool unregister_timer(int id)
            {
                bool retVal = invoke<bool>([this, id]() -> bool
                {
                    std::unique_lock<std::mutex> locker(m_mutex);
                    bool retval = false;

                    m_timers.erase(std::remove_if(m_timers.begin(), m_timers.end(), [&id, &retval](const std::pair<clock::time_point, std::shared_ptr<timer>>& time_pair) -> bool
                    {
                        if (time_pair.second->remove_client(id) == true)
                        {
                            retval = true;
                        }
                        else
                        {
                            return false;
                        }                        
                        return (time_pair.second->client_count() == 0);
                    }), m_timers.end());

                    return retval;
                });
                return retVal;
            }

            void async_loop(const std::function<bool()>& keep_looping, const std::function<void()>& func)
            {
                const std::function<bool()> local_keep_looping = keep_looping;
                const std::function<void()> local_func = func;
                if (invoke_required() == true)
                {
                    begin_invoke([this, local_keep_looping, local_func]() -> void { async_loop(local_keep_looping, local_func); });
                }
                else
                {
                    if (local_keep_looping() == true)
                    {
                        func();
                        if (disposed() == false)
                        {
                            begin_invoke([this, local_keep_looping, local_func]() -> void
                            {
                                async_loop(local_keep_looping, local_func);
                            }, nullptr, true);
                        }
                    }
                }
            }
        };
    }
}
