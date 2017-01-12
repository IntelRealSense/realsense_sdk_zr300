// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once

#define SDK_VER_MAJOR 0
#define SDK_VER_MINOR 7
#define SDK_VER_PATCH 1
#define SDK_VERSION_STRING static volatile char version_id[] = "VERSION: 0.7.1";
#define SDK_BUILD_STRING     static volatile char build[]= "BUILD DATE: " __DATE__ " " __TIME__;
#define SDK_COPYRIGHT_STRING static volatile char copyright[]="COPYRIGHT: Intel Confidential Copyright 2016";

SDK_VERSION_STRING
SDK_BUILD_STRING
SDK_COPYRIGHT_STRING
