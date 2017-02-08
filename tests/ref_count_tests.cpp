// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <memory>

#include "gtest/gtest.h"
#include "rs/core/ref_count_interface.h"
#include "rs/utils/ref_count_base.h"
#include "rs/utils/smart_ptr_helpers.h"

#include "utilities/utilities.h"

using namespace std;
using namespace rs::core;
using namespace rs::utils;

namespace mock
{
    class ref_counted_derived_interface : public ref_count_interface
    {
    public:
        virtual int get_value_type_data() const noexcept = 0;
        virtual void add_to_value_typed_data(int addition) noexcept = 0;
    protected:
        virtual ~ref_counted_derived_interface() {}
    };

    class ref_counted_derived : public rs::utils::ref_count_base<ref_counted_derived_interface>
    {
    public:
        ref_counted_derived() :
            ref_count_base(),
            m_value_typed_data(10),
            m_allocated_data(new int(20)) {}
        ref_counted_derived(int value_typed_data, int * allcated_data) :
            ref_count_base(),
            m_value_typed_data(value_typed_data),
            m_allocated_data(allcated_data) {}
        int get_value_type_data() const noexcept
        {
            return m_value_typed_data;
        }

        void add_to_value_typed_data(int addition) noexcept
        {
            m_value_typed_data += addition;
        }
    protected:
        virtual ~ref_counted_derived()
        {
            delete m_allocated_data;
        }

    private:
        int m_value_typed_data;
        int * m_allocated_data;
    };
}


GTEST_TEST(reference_count_base_tests, basic_flow) try
{
    mock::ref_counted_derived_interface * test_object = new mock::ref_counted_derived();

    ASSERT_EQ(0, test_object->release());
}CATCH_SDK_EXCEPTION()

GTEST_TEST(reference_count_base_tests, shared_ptr_wrapper) try
{
    mock::ref_counted_derived_interface * object = new mock::ref_counted_derived();
    std::shared_ptr<mock::ref_counted_derived_interface> initially_filled_shared_object(get_shared_ptr_with_releaser(object));

    std::shared_ptr<mock::ref_counted_derived_interface> initially_empty_shared_object;

    ASSERT_EQ(1, initially_filled_shared_object.use_count());
    initially_empty_shared_object = initially_filled_shared_object;
    ASSERT_EQ(2, initially_filled_shared_object.use_count());
    ASSERT_EQ(1, initially_filled_shared_object->ref_count());
    ASSERT_EQ(2, object->add_ref());

    initially_filled_shared_object.reset();

    ASSERT_EQ(0, initially_filled_shared_object.use_count());
    ASSERT_EQ(1, initially_empty_shared_object.use_count());

    ASSERT_EQ(2, initially_empty_shared_object->ref_count());
    initially_empty_shared_object.reset();

    ASSERT_EQ(1, object->ref_count());
    ASSERT_EQ(0, object->release());
}CATCH_SDK_EXCEPTION()

GTEST_TEST(reference_count_base_tests, unique_ptr_wrapper) try
{
    mock::ref_counted_derived_interface * object = new mock::ref_counted_derived();

    auto initially_filled_unique_object(get_unique_ptr_with_releaser(object));

    rs::utils::unique_ptr<mock::ref_counted_derived_interface> initially_empty_unique_object(nullptr);

    ASSERT_TRUE(initially_filled_unique_object != nullptr);
    ASSERT_EQ(1, initially_filled_unique_object->ref_count());
    ASSERT_EQ(2, object->add_ref());
    ASSERT_EQ(2, initially_filled_unique_object->ref_count());
    initially_empty_unique_object = std::move(initially_filled_unique_object);
    ASSERT_TRUE(initially_filled_unique_object == nullptr);
    ASSERT_TRUE(initially_empty_unique_object != nullptr);
    ASSERT_EQ(2, initially_empty_unique_object->ref_count());
    ASSERT_EQ(10, initially_empty_unique_object->get_value_type_data());
    ASSERT_EQ(1, initially_empty_unique_object->release());
    ASSERT_EQ(10, initially_empty_unique_object->get_value_type_data());
    ASSERT_EQ(1, initially_empty_unique_object->ref_count());
    ASSERT_EQ(1, object->ref_count());
    initially_empty_unique_object.reset();
}CATCH_SDK_EXCEPTION()
