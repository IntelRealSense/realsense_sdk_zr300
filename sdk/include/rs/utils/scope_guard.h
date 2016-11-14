// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <functional>

namespace rs
{
    namespace utils
    {
        class scope_guard
        {
        public:
            explicit scope_guard(const std::function<void(void)> &func) : m_func(func) { }
            explicit scope_guard(std::function<void(void)> &&func) : m_func(std::move(func)) { }
            ~scope_guard()
            {
                if (m_func != nullptr)
                {
                    m_func();
                }
            }
        private:
            //Non copyable, non moveable
            scope_guard(const scope_guard &) = delete;
            scope_guard &operator=(const scope_guard &) = delete;
            scope_guard(scope_guard &&other) = delete;

            std::function<void(void)> m_func;
        };
    }
}
