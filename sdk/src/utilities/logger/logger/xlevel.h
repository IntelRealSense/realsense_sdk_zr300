// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.


#pragma once

#include <log4cxx/level.h>

namespace log4cxx
{
    class XLevel : public Level
    {
        DECLARE_LOG4CXX_LEVEL(XLevel)

    public:
        enum
        {
            VERBOSE_INT = 2500,
        };

        static LevelPtr getVerbose();


        XLevel(int level, const LogString& name, int syslogEquivalent);
        /**
        Convert the string passed as argument to a level. If the
        conversion fails, then this method returns #DEBUG.
        */
        static LevelPtr toLevelLS(const LogString& sArg);

        /**
        Convert an integer passed as argument to a level. If the
        conversion fails, then this method returns #DEBUG.

        */
        static LevelPtr toLevel(int val);

        /**
        Convert an integer passed as argument to a level. If the
        conversion fails, then this method returns the specified default.
        */
        static LevelPtr toLevel(int val, const LevelPtr& defaultLevel);


        /**
        Convert the string passed as argument to a level. If the
        conversion fails, then this method returns the value of
        <code>defaultLevel</code>.
        */
        static LevelPtr toLevelLS(const LogString& sArg,
                                  const LevelPtr& defaultLevel);
    };
}

