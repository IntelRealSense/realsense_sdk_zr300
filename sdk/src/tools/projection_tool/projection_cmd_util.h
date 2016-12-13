// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "types.h"
#include "basic_cmd_util.h"

/**  @brief The projection_cmd_util class
 *
 * Command line utility with options suitable for projection tool usage.
 * Derived from basic_cmd_util to use its public methods.
 */
class projection_cmd_util : public rs::utils::basic_cmd_util
{
public:
    /** @brief projection_cmd_util
     *
     * Constructor. Sets the relevant command line options.
     */
    projection_cmd_util() :
        basic_cmd_util(false)
    {
        add_option("-h --h -help --help -?", "show help");
    
        add_multi_args_option_safe("-dconf", "set depth profile - [<width>-<height>-<fps>]", 3, '-', "", "628-468-30");
        add_single_arg_option("-dpf", "set depth streams pixel format", "z16", "z16");
    
        add_multi_args_option_safe("-cconf", "set color stream profile - [<width>-<height>-<fps>]", 3, '-', "", "640-480-30");
        add_single_arg_option("-cpf", "set color stream pixel format", "rgb8 rgba8 bgr8 bgra8", "rgba8");
    
        add_multi_args_option_safe("-fconf", "set fisheye stream profile - [<width>-<height>-<fps>]", 3, '-', "", "640-480-30");
        add_single_arg_option("-fpf", "set fisheye stream pixel format", "raw8", "raw8");

        add_single_arg_option("-pb -playback", "set playback file path");

        set_usage_example("-cconf 640-480-30 -cpf rgb8\n\n"
                          "The following command will configure the camera to\n"
                          "show color stream of VGA resolution at 30 frames\n"
                          "per second in rgb8 pixel format.\n"
                          "Color, Depth and Fisheye streams MUST be available in case of prerecorded clips.\n"
                          "Color, Depth, Fisheye streams and World image are ALWAYS shown.\n"
                          "Other projection-generated images can be also viewed using specific keyboard keys.\n"
                          "GUI help message is always shown in the main window.\n");
    }
};
