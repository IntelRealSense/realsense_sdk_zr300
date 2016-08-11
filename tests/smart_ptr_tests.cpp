// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <future>
#include <memory>

#include "gtest/gtest.h"
#include "rs/utils/smart_ptr.h"

using namespace std;
using namespace rs::utils;

namespace mock
{
    class test_data
    {
    public:
        test_data(const test_data &) = delete;
        test_data & operator=(const test_data&) = delete;
        test_data() = default;
        test_data(int x) : m_x(x) {}
        virtual int get_x() const noexcept { return m_x; }
        void add_to_x(int addition) noexcept { m_x += addition; }
        virtual ~test_data() {}
    private:
        int m_x;
    };
}

GTEST_TEST(smart_ptr_tests, basic_ptr)
{
    smart_ptr<mock::test_data> original_data(new mock::test_data());
    ASSERT_EQ(1, original_data.use_count());
    smart_ptr<mock::test_data> assigned_data = original_data;

    ASSERT_EQ(original_data.get(), assigned_data.get());
    ASSERT_EQ(2, original_data.use_count());
    ASSERT_EQ(2, assigned_data.use_count());

    original_data.reset();
    ASSERT_EQ(0, original_data.use_count());
    ASSERT_EQ(1, assigned_data.use_count());
}

GTEST_TEST(smart_ptr_tests, copy_ctor)
{
    smart_ptr<mock::test_data> original_data(new mock::test_data(1));
    smart_ptr<mock::test_data> copied_ctor(original_data);

    ASSERT_EQ(original_data.get(), copied_ctor.get());
    ASSERT_NE(nullptr, original_data.get());
    ASSERT_EQ(2, original_data.use_count());
    ASSERT_EQ(2, copied_ctor.use_count());
}

GTEST_TEST(smart_ptr_tests, move_ctor)
{
    smart_ptr<mock::test_data> original_data(new mock::test_data(1));
    smart_ptr<mock::test_data> moved_ctor(std::move(original_data));

    ASSERT_EQ(nullptr, original_data.get());
    ASSERT_NE(nullptr, moved_ctor.get());
    ASSERT_EQ(1, moved_ctor.use_count());
}

GTEST_TEST(smart_ptr_tests, assignment_op)
{
    smart_ptr<mock::test_data> original_data(new mock::test_data(1));
    smart_ptr<mock::test_data> assignment_op_data = original_data;

    ASSERT_EQ(assignment_op_data.get(), original_data.get());
    ASSERT_NE(nullptr, original_data.get());
    ASSERT_EQ(2, original_data.use_count());
    ASSERT_EQ(2, assignment_op_data.use_count());
}

GTEST_TEST(smart_ptr_tests, move_op)
{
    smart_ptr<mock::test_data> original_data(new mock::test_data(1));
    smart_ptr<mock::test_data> move_op_data = std::move(original_data);

    ASSERT_NE(original_data.get(), move_op_data.get());
    ASSERT_EQ(nullptr, original_data.get());
    ASSERT_EQ(1, move_op_data.use_count());
}

GTEST_TEST(smart_ptr_tests, equality)
{
    smart_ptr<mock::test_data> instance1_data(new mock::test_data(1));
    smart_ptr<mock::test_data> instance2_data(new mock::test_data(1));
    smart_ptr<mock::test_data> copied_instance1_data(instance1_data);

    ASSERT_FALSE(instance1_data == instance2_data);
    ASSERT_TRUE(instance1_data != instance2_data);
    ASSERT_TRUE(copied_instance1_data == instance1_data);
    ASSERT_FALSE(copied_instance1_data != instance1_data);
}

GTEST_TEST(smart_ptr_tests, deref)
{
    smart_ptr<mock::test_data> original_data(new mock::test_data(1));
    mock::test_data & raw_data = *original_data;

    ASSERT_EQ(1, raw_data.get_x());
    ASSERT_EQ(&raw_data, original_data.get());
}

GTEST_TEST(smart_ptr_tests, bool_operator)
{
    smart_ptr<mock::test_data> filled_data(new mock::test_data(1));
    smart_ptr<mock::test_data> empty_data;
    if(!filled_data) { FAIL(); }
    if(empty_data) { FAIL(); }
}

GTEST_TEST(smart_ptr_tests, reset_void)
{
    smart_ptr<mock::test_data> original_data(new mock::test_data(1));
    smart_ptr<mock::test_data> assigned_data = original_data;
    original_data.reset();

    ASSERT_NE(original_data.get(), assigned_data.get());
    ASSERT_EQ(1, assigned_data.use_count());
    ASSERT_EQ(nullptr, original_data.get());
}

GTEST_TEST(smart_ptr_tests, reset_raw_ptr)
{
    smart_ptr<mock::test_data> original_data(new mock::test_data(1));
    smart_ptr<mock::test_data> assigned_data = original_data;
    original_data.reset(new mock::test_data(2));

    ASSERT_NE(original_data.get(), assigned_data.get());
    ASSERT_EQ(1, assigned_data.use_count());
    ASSERT_EQ(1, assigned_data->get_x());
    ASSERT_EQ(1, original_data.use_count());
    ASSERT_EQ(2, original_data->get_x());
}

GTEST_TEST(smart_ptr_tests, swap)
{
    smart_ptr<mock::test_data> original_data(new mock::test_data(1));
    smart_ptr<mock::test_data> initially_empty_data;

    initially_empty_data.swap(original_data);
    ASSERT_EQ(1, initially_empty_data.use_count());
    ASSERT_EQ(1, initially_empty_data->get_x());
    ASSERT_EQ(0, original_data.use_count());
    ASSERT_EQ(nullptr, original_data.get());

    initially_empty_data.swap(original_data);
    ASSERT_EQ(1, original_data.use_count());
    ASSERT_EQ(1, original_data->get_x());
    ASSERT_EQ(0, initially_empty_data.use_count());
    ASSERT_EQ(nullptr, initially_empty_data.get());
}

GTEST_TEST(smart_ptr_tests, alias_ctor)
{
    class derived_test_data : public mock::test_data
    {
    public:
        derived_test_data(int y) : mock::test_data(), m_y(y) {}
        int get_y() {return m_y;}
        virtual int get_x() const noexcept { return -1; }
    private:
        int m_y;
    };

    smart_ptr<derived_test_data> derived_data(new derived_test_data(1));
    smart_ptr<mock::test_data> base_type_data(derived_data);

    ASSERT_EQ(2, derived_data.use_count());
    ASSERT_EQ(-1, derived_data->get_x());
    ASSERT_EQ(2, base_type_data.use_count());
    ASSERT_EQ(-1, base_type_data->get_x());
    ASSERT_EQ(1, derived_data->get_y());
    derived_data.reset();
    ASSERT_EQ(0, derived_data.use_count());
    ASSERT_EQ(1, base_type_data.use_count());

    smart_ptr<const mock::test_data> const_data = base_type_data;

    ASSERT_EQ(2, const_data.use_count());
    ASSERT_EQ(-1, const_data->get_x());
    //const_data->add_to_x(3); not a const function.

}

