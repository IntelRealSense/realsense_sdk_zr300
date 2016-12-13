// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <cassert>
#include <iostream>
#include "cmd_base.h"

namespace
{
    size_t get_option_tags_length(rs::utils::cmd_option option)
    {
        size_t tags_length = 0;
        for(auto tag : option.m_tags)
        {
            tags_length += tag.size() + 1;
        }
        return tags_length;
    }
}

namespace rs
{
    namespace utils
    {
        cmd_base::cmd_base()
        {

        }
        bool cmd_base::add_option(std::string tags, std::string description)
        {
            return set_option(cmd_option::option_type::no_arg, tags, description, 0, 0,"", "");
        }

        bool cmd_base::add_single_arg_option(std::string tags, std::string description, std::string optional_values, std::string default_value)
        {
            return set_option(cmd_option::option_type::single_arg, tags, description, 1, 0,optional_values, default_value);
        }
		
        bool cmd_base::add_multi_args_option_safe(std::string tags, std::string description, int expected_args_count, char delimiter, std::string optional_values, std::string default_value)
        {
            return set_option(cmd_option::option_type::multy_args, tags, description, expected_args_count, delimiter, optional_values, default_value);
        }


        bool cmd_base::set_option(cmd_option::option_type type, std::string tags, std::string description, int expected_args_count, char delimiter, std::string optional_values, std::string default_value)
        {
            assert(!tags.empty());
            auto split_tags = split(tags, ' ');
            assert(find(split_tags) == -1);
            cmd_option opt = {};
            opt.m_option_type = type;
            opt.m_tags = split_tags;
            opt.m_description = description;
            opt.m_delimiter = delimiter;
            opt.m_option_args_legal_values = split(optional_values, ' ');
            opt.m_default_value = default_value;
            opt.m_expected_args_count = expected_args_count;
            m_options.push_back(opt);
            return true;
        }

        std::string cmd_base::get_help()
        {
            std::stringstream rv;
            size_t max_opt_length = 0;
            size_t max_desc_length = 0;
            for(auto opt : m_options)
            {
                auto opt_length = get_option_tags_length(opt);
                if(opt_length > max_opt_length)
                    max_opt_length = opt_length;
                if(opt.m_description.size() > max_desc_length)
                    max_desc_length = opt.m_description.size();
            }
            if(m_usage_example != "")
            {
                rv << std::endl << "Usage:" << std::endl;
                rv << m_usage_example << std::endl;
            }
            rv << std::endl << "Options:" << std::endl;
            for(auto opt : m_options)
            {
                std::stringstream line;
                line << "\t";
                for(auto tag : opt.m_tags)
                {
                    line << tag << " ";
                }
                auto tags_length = get_option_tags_length(opt);
                for (size_t i = 0; i < (max_opt_length-tags_length); i++)
                    line << " ";
                line << "\t" << opt.m_description;
                for (size_t i = 0; i < (max_desc_length-opt.m_description.size()); i++)
                    line << " ";
                if(opt.m_delimiter != 0)
                    line <<"\tdelimiter: " << "\"" << opt.m_delimiter << "\"";
                if(opt.m_option_args_legal_values.size() > 0 )
                {
                    line << "\tlegal values:";
                    for(auto lv : opt.m_option_args_legal_values)
                    {
                        line << " " << lv;
                    }
                }
                if(opt.m_default_value != "")
                    line << "\tdefault value: " << opt.m_default_value;
                line << std::endl;
                rv << line.str();
            }
            return rv.str();
        }

        bool cmd_base::get_cmd_option(std::string str, cmd_option &option)
        {
            auto split_strs = split(str, ' ');
            auto index = find(split_strs);
            if(index == -1)return false;
            option = m_options[index];
            for(auto s : split_strs)
                if(std::find(m_args.begin(), m_args.end(), s) != m_args.end()) return true;
            return false;
        }

        bool cmd_base::parse(int argc, char* argv[])
        {
            try
            {
                if(argc == 1) return false;
                for(int i = 0; i < argc; i++)
                {
                    m_args.push_back(argv[i]);
                }
                m_args.erase(m_args.begin());

                for(auto it = m_args.begin(); it != m_args.end(); ++it)
                {
                    auto index = find(*it);
                    if(index == -1)
                        throw "failed to parse argument, value: " + *it;
                    cmd_option & opt = m_options[index];
                    switch(opt.m_option_type)
                    {
                        case cmd_option::option_type::no_arg: continue;
                        case cmd_option::option_type::single_arg:
                        {
                            if((it+1) == m_args.end() || find(*(it+1)) != -1)
                                throw "missing argument for: " + *(it);
                            opt.m_option_args_values.push_back(*(++it));
                            opt.check_validity();
                            continue;
                        }
                        case cmd_option::option_type::multy_args:
                        {
                            if(opt.m_delimiter != ' ')
                            {
                                auto arg = *it;
                                if(++it == m_args.end())
                                    throw "failed to parse argument, value: " + arg;
                                opt.m_option_args_values = split(*it, opt.m_delimiter);
                                opt.check_validity();
                                continue;
                            }
                            while(it != m_args.end())
                            {
                                if(find(*(it+1)) != -1 || (it+1) == m_args.end())break;
                                opt.m_option_args_values.push_back(*(++it));
                            }
                            opt.check_validity();
                        }
                    }
                }
                return true;
            }
            catch(std::string e)
            {
                std::cout << e << std::endl;
                return false;
            }
        }

        std::string cmd_base::get_selection()
        {
            std::stringstream rv;
            for(auto arg : m_args)
            {
                auto index = find(arg);
                if(index == -1)continue;
                cmd_option opt = m_options[index];
                rv << std::endl << opt.m_description ;
                if(opt.m_option_args_values.size() > 0 )
                {
                    for(auto val : opt.m_option_args_values)
                        rv << " " << val;
                }
            }
            rv << std::endl;
            return rv.str();
        }

        void cmd_base::set_usage_example(std::string usage_example)
        {
            m_usage_example = usage_example;
        }

        int cmd_base::find(std::vector<std::string> strs)
        {
            for(auto str : strs)
            {
                auto index = find(str);
                if(index != -1) return index;
            }
            return -1;
        }

        int cmd_base::find(std::string str)
        {
            for(int i = 0; i < (int)m_options.size(); i++)
            {
                for(auto tag : m_options[i].m_tags)
                {
                    if(tag == str)
                        return i;
                }
            }
            return -1;
        }

        std::vector<std::string> & cmd_base::split(const std::string &s, char delim, std::vector<std::string> &elems)
        {
            std::stringstream ss(s);
            std::string item;
            while (getline(ss, item, delim))
            {
                elems.push_back(item);
            }
            return elems;
        }

        std::vector<std::string> cmd_base::split(const std::string &s, char delim)
        {
            std::vector<std::string> elems;
            split(s, delim, elems);
            return elems;
        }
    }
}
