// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include <vector>
#include <exception>
#include <ostream>

namespace test_utils
{
    /**
     * @brief Splits a string into substrings that are based on the characters in the delimiter string
     *
     * @param str The string to split
     * @param delimiters A character array that delimits the substrings in the string
     * @return A vector whose elements contain the substrings from the string instance that are delimited by one or more characters in separator.
     */
    static std::vector<std::string> split_str(const std::string& str, const std::string& delimiters)
    {
        std::vector<std::string> tokens;
        
        std::string::size_type last_pos = str.find_first_not_of(delimiters, 0);
        std::string::size_type pos     = str.find_first_of(delimiters, last_pos);
    
        while (pos != std::string::npos || last_pos != std::string::npos)
        {
            tokens.push_back(str.substr(last_pos, pos - last_pos));
            last_pos = str.find_first_not_of(delimiters, pos);
            pos = str.find_first_of(delimiters, last_pos);
        }
        
        return tokens;
    }
    
    /**
     * @brief The version class represents a version number of a component
     *
     * A Valid version number is one of the following patterns:
     *  1) major.minor
     *  2) major.minor.build
     *  3) major.minor.build.revision
     * Where "major", "minor", "build" and "revision" are non-negative integers
     */
    class version
    {
    public:
        version() : m_major(0), m_minor(0) {}
        
        version(int major, int minor)
        {
            if (major < 0)
            {
                throw std::invalid_argument("major");
            }
            if (minor < 0)
            {
                throw std::invalid_argument("minor");
            }
            
            m_major = major;
            m_minor = minor;
        }
        
        version(int major, int minor, int build)
        {
            if (major < 0)
            {
                throw std::invalid_argument("major");
            }
            if (minor < 0)
            {
                throw std::invalid_argument("minor");
            }
            if (build < 0)
            {
                throw std::invalid_argument("build");
            }
            m_major = major;
            m_minor = minor;
            m_build = build;
        }
        
        version(int major, int minor, int build, int revision)
        {
            if (major < 0)
            {
                throw std::invalid_argument("major");
            }
            if (minor < 0)
            {
                throw std::invalid_argument("minor");
            }
            if (build < 0)
            {
                throw std::invalid_argument("build");
            }
            if (revision < 0)
            {
                throw std::invalid_argument("revision");
            }
            m_major = major;
            m_minor = minor;
            m_build = build;
            m_revision = revision;
        }
        
        version(const std::string& version_str)
        {
            if (try_parse_version(version_str, *this) == false)
            {
                throw std::invalid_argument("version_str");
            }
        }
        
        int major()    const { return m_major; }
        int minor()    const { return m_minor; }
        int build()    const { return m_build; }
        int revision() const { return m_revision; }
       
        int compare(const version& other) const
        {
            if (m_major != other.m_major)
            {
                if (m_major > other.m_major)
                {
                    return 1;
                }
                else
                {
                    return -1;
                }
            }
            if (m_minor != other.m_minor)
            {
                if (m_minor > other.m_minor)
                {
                    return 1;
                }
                else
                {
                    return -1;
                }
            }
            if (m_build != other.m_build)
            {
                if (m_build > other.m_build)
                {
                    return 1;
                }
                else
                {
                    return -1;
                }
            }
    
            if (m_revision != other.m_revision)
            {
                if (m_revision > other.m_revision)
                {
                    return 1;
                }
                else
                {
                    return -1;
                }
            }
    
            return 0;
        }
    
        bool operator==(const version& other) const
        {
            return m_major == other.m_major &&
                m_minor == other.m_minor &&
                m_build == other.m_build &&
                m_revision == other.m_revision;
        }
    
        bool operator!=(const version& other) const
        {
            return !(*this == other);
        }
    
        bool operator<(const version& other) const
        {
            return this->compare(other) < 0;
        }
    
        bool operator<=(const version& other) const
        {
            return this->compare(other) <= 0;
        }
    
        bool operator>(const version& other) const
        {
            return other < *this;
        }
    
        bool operator>=(const version& other) const
        {
            return other <= *this;
        }
    
        static bool try_parse_version(const std::string &version_str, version &v)
        {
            if (version_str.empty())
            {
                return false;
            }
        
            std::vector<std::string> version_parts = split_str(version_str, ".");
            if (version_parts.size() < 2 || version_parts.size() > 4)
            {
                return false;
            }
            
            try
            {
                int major = std::stoi(version_parts[0]);
                int minor = std::stoi(version_parts[1]);
                int build = 0;
                int revision = 0;
                if (version_parts.size() > 3)
                {
                    build = std::stoi(version_parts[2]);
                    revision = std::stoi(version_parts[3]);
                    v = version(major, minor, build, revision);
                }else if(version_parts.size() > 2)
                {
                    build = std::stoi(version_parts[2]);
                    v = version(major, minor, build);
                }
                else
                {
                    v = version(major, minor);
                }
            }
            catch (const std::exception &e)
            {
                return false;
            }
            
            return true;
        }
        
    private:
        int m_major = -1;
        int m_minor = -1;
        int m_build = -1;
        int m_revision = -1;
    };
    
    static std::ostream& operator<<(std::ostream& os, const version& version)
    {
        os << version.major() << "." << version.minor();
        if(version.build() >= 0)
        {
            os << "." << version.build();
        }
        if(version.revision() >= 0)
        {
            os << "." << version.revision();
        }
        return os;
    }
}