// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "gtest/gtest.h"
#include "rs/utils/cyclic_array.h"
#include "utilities/version.h"
#include "utilities/utilities.h"

using namespace std;
using namespace rs::utils;
using namespace test_utils;

TEST(version_test, test_constructor) try
{
    ASSERT_NO_THROW(version());
    ASSERT_EQ(version(), version(0,0));
    ASSERT_THROW(version(0, -1), std::invalid_argument);
    ASSERT_THROW(version(0, -2), std::invalid_argument);
    ASSERT_THROW(version(-2, 0), std::invalid_argument);
    ASSERT_THROW(version(-1, 0), std::invalid_argument);
    ASSERT_THROW(version(-1, -1), std::invalid_argument);
    ASSERT_THROW(version(-2, -3), std::invalid_argument);

    ASSERT_THROW(version(-1, 0, 0), std::invalid_argument);
    ASSERT_THROW(version(0, -1, 0), std::invalid_argument);
    ASSERT_THROW(version(0, 0, -1), std::invalid_argument);
    ASSERT_THROW(version(-1, 0, 0, 0), std::invalid_argument);
    ASSERT_THROW(version(0, -1, 0, 0), std::invalid_argument);
    ASSERT_THROW(version(0, 0, -1, 0), std::invalid_argument);
    ASSERT_THROW(version(0, 0, 0, -1), std::invalid_argument);

    ASSERT_THROW(version(nullptr), std::exception);
    ASSERT_THROW(version(""), std::invalid_argument);
    ASSERT_THROW(version("1"), std::invalid_argument);
    ASSERT_THROW(version("1."), std::invalid_argument);
    ASSERT_THROW(version("1.-1"), std::invalid_argument);
    ASSERT_THROW(version("-1.1"), std::invalid_argument);

    ASSERT_NO_THROW(version("1.1."));

    ASSERT_EQ(version(1, 1), version(1, 1));
    ASSERT_EQ(version("1.1."), version(1, 1));
    ASSERT_EQ(version("1.1"), version(1, 1));
    ASSERT_EQ(version("1.2.3"), version(1, 2, 3));
    ASSERT_EQ(version("1.2.3.4"), version(1, 2, 3, 4));

    ASSERT_NE(version("1.2.3.0"), version(1, 2, 3));
}CATCH_SDK_EXCEPTION()

TEST(cyclic_array, zero_length_array) try
{
    cyclic_array<int> zero_length_array;
    ASSERT_EQ(zero_length_array.size(), 0u);
    int x;
    ASSERT_THROW(zero_length_array.push_back(x), std::out_of_range);
    ASSERT_THROW(zero_length_array.back(), std::out_of_range);
    
    
    ASSERT_THROW(zero_length_array.front(), std::out_of_range);
    
    zero_length_array.pop_back();
    ASSERT_EQ(zero_length_array.size(), 0u);
    
    zero_length_array.pop_front();
    ASSERT_EQ(zero_length_array.size(), 0u);
        
}CATCH_SDK_EXCEPTION()

TEST(cyclic_array, single_element_array) try
{
    cyclic_array<int> single_element_array(1);
    ASSERT_EQ(single_element_array.size(), 0u);
    single_element_array.pop_front();
    ASSERT_EQ(single_element_array.size(), 0u);
    single_element_array.pop_back();
    ASSERT_EQ(single_element_array.size(), 0u);
    
    int e = 12;
    single_element_array.push_back(e);
    ASSERT_EQ(single_element_array.size(), 1u);
    
    ASSERT_EQ(single_element_array.back(), e);
    ASSERT_EQ(single_element_array.front(), e);
    
    single_element_array.pop_front();
    ASSERT_EQ(single_element_array.size(), 0u);
    
    e = 13;
    single_element_array.push_back(e);
    ASSERT_EQ(single_element_array.size(), 1u);
    ASSERT_EQ(single_element_array.back(), e);
    ASSERT_EQ(single_element_array.front(), e);
    
    
    single_element_array.pop_back();
    ASSERT_EQ(single_element_array.size(), 0u);
    
    e = 14;
    single_element_array.push_back(e);
    ASSERT_EQ(single_element_array.size(), 1u);
    ASSERT_EQ(single_element_array.back(), e);
    ASSERT_EQ(single_element_array.front(), e);
    
    single_element_array.pop_front();
    ASSERT_EQ(single_element_array.size(), 0u);
    e=15;
    single_element_array.push_back(e);
    e=16;
    single_element_array.push_back(e);
    ASSERT_EQ(single_element_array.size(), 1u);
    ASSERT_EQ(single_element_array.back(), e);
    ASSERT_EQ(single_element_array.front(), e);
}CATCH_SDK_EXCEPTION()


TEST(cyclic_array, cyclic_array_test) try
{
    cyclic_array<int> array(3);
    ASSERT_EQ(array.size(), 0u);
    int x = 1;
    array.push_back(x);
    //array: [1]
    ASSERT_EQ(array.size(), 1u);
    x = 2;
    array.push_back(x);
    //array: [2 , 1]
    ASSERT_EQ(array.size(), 2u);
    x = 3;
    array.push_back(x);
    //array: [3 , 2 , 1]
    ASSERT_EQ(array.size(), 3u);
    ASSERT_EQ(array.front(), 1);
    ASSERT_EQ(array.back(), 3);
    
    x = 4;
    array.push_back(x);
    //array: [4 , 3 , 2]
    // ASSERT_EQ(array.size(), 3u);
    
    ASSERT_EQ(array.front(), 2);
    ASSERT_EQ(array.back(), 4);
    
    
    x = 5;
    array.push_back(x);
    //array: [5 , 4 , 3]
    ASSERT_EQ(array.size(), 3u);
    
    ASSERT_EQ(array.front(), 3);
    ASSERT_EQ(array.back(), 5);
    
    x = 6;
    array.push_back(x);
    //array: [6 , 5 , 4]
    ASSERT_EQ(array.size(), 3u);
    
    ASSERT_EQ(array.front(), 4);
    ASSERT_EQ(array.back(), 6);
    
    x = 7;
    array.push_back(x);
    //array: [7 , 6 , 5]
    ASSERT_EQ(array.size(), 3u);
    
    ASSERT_EQ(array.front(), 5);
    ASSERT_EQ(array.back(), 7);
    
    
    array.pop_front();
    //array: [7 , 6]
    ASSERT_EQ(array.size(), 2u);
    ASSERT_EQ(array.front(), 6);
    ASSERT_EQ(array.back(), 7);
    
    
    //array: [8, 7 , 6]
    x = 8;
    array.push_back(x);
    ASSERT_EQ(array.size(), 3u);
    ASSERT_EQ(array.front(), 6);
    ASSERT_EQ(array.back(), 8);
    
    array.pop_front();
    //array: [8 , 7]
    ASSERT_EQ(array.size(), 2u);
    ASSERT_EQ(array.front(), 7);
    ASSERT_EQ(array.back(), 8);
    
    array.pop_front();
    //array: [8]
    ASSERT_EQ(array.size(), 1u);
    ASSERT_EQ(array.front(), 8);
    ASSERT_EQ(array.back(), 8);
    
    array.pop_front();
    //array: []
    ASSERT_EQ(array.size(), 0u);
    ASSERT_THROW(array.front(), std::out_of_range);
    ASSERT_THROW(array.back(), std::out_of_range);
    
    array.pop_front();
    //array: []
    ASSERT_EQ(array.size(), 0u);
    ASSERT_THROW(array.front(), std::out_of_range);
    ASSERT_THROW(array.back(), std::out_of_range);
    
}CATCH_SDK_EXCEPTION()
    
    
