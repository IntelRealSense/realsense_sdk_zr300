// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.


#include "xlevel.h"
#include <log4cxx/helpers/stringhelper.h>

using namespace log4cxx;
using namespace log4cxx::helpers;


IMPLEMENT_LOG4CXX_LEVEL(XLevel)

XLevel::XLevel(int level1, const LogString& name1, int syslogEquivalent1) :
    Level(level1, name1, syslogEquivalent1)
{
}

LevelPtr XLevel::getVerbose()
{
    static const LevelPtr trace(
        new XLevel(XLevel::VERBOSE_INT, LOG4CXX_STR("VERBOSE"), 7));
    return trace;
}

LevelPtr XLevel::toLevelLS(const LogString& sArg)
{
    return toLevelLS(sArg, getVerbose());
}

LevelPtr XLevel::toLevel(int val)
{
    return toLevel(val, getVerbose());
}

LevelPtr XLevel::toLevel(int val, const LevelPtr& defaultLevel)
{
    switch (val)
    {
        case VERBOSE_INT:
            return getVerbose();
        default:
            return Level::toLevel(val, defaultLevel);
    }
}

LevelPtr XLevel::toLevelLS(const LogString& sArg,
                           const LevelPtr& defaultLevel)
{
    if (sArg.empty())
    {
        return defaultLevel;
    }

    if (StringHelper::equalsIgnoreCase(sArg, LOG4CXX_STR("VERBOSE"),
                                       LOG4CXX_STR("verbose")))
    {
        return getVerbose();
    }

    return Level::toLevel(sArg, defaultLevel);
}

