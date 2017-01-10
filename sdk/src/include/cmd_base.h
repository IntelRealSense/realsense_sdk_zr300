// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

namespace rs
{
    namespace utils
    {
        class cmd_option
        {
        public:
            enum option_type
            {
                no_arg,
                single_arg,
                multy_args
            };

            void check_validity()
            {
                if(m_expected_args_count >= 0 && (int)m_option_args_values.size() != m_expected_args_count)
                    throw "args count doesn't match the expected args count: " +
                    std::to_string(m_option_args_values.size()) + " vs. " + std::to_string(m_expected_args_count);
                if(m_option_args_legal_values.size() ==  0) return;
                for(auto str : m_option_args_values)
                {
                    auto sts = std::find(m_option_args_legal_values.begin(), m_option_args_legal_values.end(), str) != m_option_args_legal_values.end();
                    if(!sts) throw "illegal value, value: " + str;
                }
            }

            std::vector<std::string> m_tags;
            std::string m_description;
            char m_delimiter;
            std::vector<std::string> m_option_args_values;
            std::vector<std::string> m_option_args_legal_values;
            std::string m_default_value;
            option_type m_option_type;
            int m_expected_args_count;
        };

        class cmd_base
        {
        public:
            cmd_base();

            /**
            @brief Add a new option with out option arguments.
            @param[in] tags  Space seperated tags list for the new option.
            @param[in] description  Description string of the new option.
            @return TRUE    In case option tags are not empty and doesn't already exist.
            */
            bool add_option(std::string tags, std::string description);

            /**
            @brief Add a new option with single option arguments.
            @param[in] tags  Space seperated tags list for the new option.
            @param[in] description  Description string of the new option.
            @param[in] optional_values  Space seperated legal args list for the new option.(Optional)
            @param[in] default_value  The option argument that will be used in case the user did not provide one.
            @return TRUE    In case option tags are not empty and doesn't already exist.
            */
            bool add_single_arg_option(std::string tags, std::string description, std::string optional_values = "", std::string default_value = "");

            /**
            @brief Add a new option with n option arguments.
            @param[in] tags  Space seperated tags list for the new option.
            @param[in] description  Description string of the new option.
            @param[in] expected_args_count  Specifys the option arguments list size.
            @param[in] delimiter  Specify the char value that seperate between the option arguments.
            @param[in] optional_values  Space seperated legal args list for the new option.(Optional)
            @param[in] default_value  The option argument that will be used in case the user did not provide one.
            @return TRUE    In case option tags are not empty and doesn't already exist.
            */
            bool add_multi_args_option_safe(std::string tags, std::string description, int expected_args_count, char delimiter, std::string optional_values = "", std::string default_value = "");

            /**
            @brief Returns formated string of all available options.
            */
            std::string get_help();

            /**
            @brief Returns formated string of all available options.
            @param[in] str  Space seperated tags list of the requsted option.
            @param[out] option  The requsted cmd_option.
            @return TRUE    In case option exists and enabled by the user.
            */
            bool get_cmd_option(std::string str, cmd_option & option);

            /**
            @brief Returns formated string of all available options.
            @param[in] argc  Application arguments count.
            @param[in] argv  Application arguments array.
            @return TRUE    In case all arguments that were provided by the user are valid.
            */
            bool parse(int argc, char* argv[]);

            /**
            @brief Returns formated string of the user selected options.
            */
            std::string get_selection();

            /**
            @brief Set usage example string of the available option that will be presented by "get_help" function.
            @param[in] usage_example  Application usage example string.
            */
            void set_usage_example(std::string usage_example);

        private:
            bool set_option(cmd_option::option_type type, std::string tags, std::string description, int expected_args_count, char delimiter, std::string optional_values, std::string default_value);
            std::vector<std::string> m_args;
            std::vector<cmd_option> m_options;
            std::string m_usage_example;

            int find(std::vector<std::string> strs);
            int find(std::string str);
            static std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
            static std::vector<std::string> split(const std::string &s, char delim);
        };
    }
}
