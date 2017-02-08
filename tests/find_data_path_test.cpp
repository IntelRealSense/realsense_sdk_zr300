// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <limits.h>
#include <stdlib.h>

#include "gtest/gtest.h"

#include "utilities/utilities.h"
#include "rs/utils/data_path_utility.h"

using namespace std;

class find_data_path_fixture : public testing::Test
{
public:
    find_data_path_fixture() {}


protected:
    //common initialization
    static void SetUpTestCase()
    {
    }

    //For each test case
    virtual void SetUp()
    {
        ASSERT_GE(readlink("/proc/self/exe", curent_dir, PATH_MAX-1), 1);
        *strrchr(curent_dir, '/') = '\0';
        strcat ( curent_dir, "/data" );

        Dl_info dl_info;
        ASSERT_TRUE(dladdr((void*)"my_idlopen_init", &dl_info));
        snprintf(curent_dir_fill_path, PATH_MAX, "%s", dl_info.dli_fname);
        *strrchr(curent_dir_fill_path, '/') = '\0';
        strcat ( curent_dir_fill_path, "/data" );

    }

    //For each test case
    virtual void TearDown()
    {
         printf( "'%s' -' IS TARGET DIRECTORY.\n", path);

         int rez = unsetenv("RS_SDK_DATA_PATH");
         if(rez == -1) ASSERT_TRUE(false);
         rez = system("rm -r /opt/intel/rssdk");
         if(rez == -1) ASSERT_TRUE(false);

         char tmp[PATH_MAX];
         snprintf(tmp, PATH_MAX,"rm -r  %s", curent_dir);
         rez = system(tmp);
         if(rez == -1) ASSERT_TRUE(false);
    }

    //final step
    static void TearDownTestCase()
    {
    }

    char path[PATH_MAX];
    char curent_dir[PATH_MAX];
    char curent_dir_fill_path[PATH_MAX];

    static void call_mkdir(const char *dir) {
            char tmp[PATH_MAX];
            snprintf(tmp, PATH_MAX,"mkdir -p %s",dir);

            int rez = system(tmp);
            if(rez == -1) ASSERT_TRUE(false);
    }

};


TEST_F(find_data_path_fixture, basic_false) try
{
    rs::utils::data_path get_data;

    ASSERT_EQ(get_data.get_path(path, PATH_MAX), -1);
}CATCH_SDK_EXCEPTION()

TEST_F(find_data_path_fixture, basic_false_lenght) try
{
    int a = setenv("RS_SDK_DATA_PATH","/test/test/test",1);
    rs::utils::data_path get_data;

    ASSERT_EQ(get_data.get_path(path, 1), (int)strlen("/test/test/test"));
}CATCH_SDK_EXCEPTION()

TEST_F(find_data_path_fixture, basic_false_NULL) try
{
    int a = setenv("RS_SDK_DATA_PATH","/test/test/test",1);
    rs::utils::data_path get_data;

    ASSERT_EQ(get_data.get_path(NULL, PATH_MAX), (int)strlen("/test/test/test"));
}CATCH_SDK_EXCEPTION()

TEST_F(find_data_path_fixture, basic_env_variable) try
{
    int a = setenv("RS_SDK_DATA_PATH","/test/test/test",1);

    rs::utils::data_path get_data;
    ASSERT_TRUE(get_data.get_path(path, PATH_MAX));
    ASSERT_EQ(0, strcmp (path, "/test/test/test"));
}CATCH_SDK_EXCEPTION()

TEST_F(find_data_path_fixture, DISABLED_basic_opt_folder) try
{

    call_mkdir("/opt/intel/rssdk/data");

    rs::utils::data_path get_data;
    ASSERT_TRUE(get_data.get_path(path, PATH_MAX));
    ASSERT_EQ(0, strcmp (path, "/opt/intel/rssdk/data"));
}CATCH_SDK_EXCEPTION()



TEST_F(find_data_path_fixture, basic_opt_current_folder) try
{
    call_mkdir(curent_dir);

    rs::utils::data_path get_data;
    ASSERT_TRUE(get_data.get_path(path, PATH_MAX));

    printf("%s --- %s", path, curent_dir_fill_path);
    ASSERT_EQ(0, strcmp (path, curent_dir_fill_path));
}CATCH_SDK_EXCEPTION()

TEST_F(find_data_path_fixture, basic_env_variable_and_opt) try
{
    int a = setenv("RS_SDK_DATA_PATH","/test/test/test",1);
    call_mkdir("/opt/intel/rssdk/data");

    rs::utils::data_path get_data;
    ASSERT_TRUE(get_data.get_path(path, PATH_MAX));
    ASSERT_EQ(0, strcmp (path, "/test/test/test"));
}CATCH_SDK_EXCEPTION()

TEST_F(find_data_path_fixture, basic_env_variable_and_opt_and_cur_folder) try
{
    int a = setenv("RS_SDK_DATA_PATH","/test/test/test",1);
    call_mkdir("/opt/intel/rssdk/data");
    call_mkdir(curent_dir);

    rs::utils::data_path get_data;
    ASSERT_TRUE(get_data.get_path(path, PATH_MAX));
    ASSERT_EQ(0, strcmp (path, curent_dir_fill_path));
}CATCH_SDK_EXCEPTION()
