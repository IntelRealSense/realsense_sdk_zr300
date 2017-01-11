# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2016 Intel Corporation. All Rights Reserved.

import sys
import argparse
import streaming
from PIL import Image

#setup arguments
parser = argparse.ArgumentParser()
parser.add_argument('--color_width' , type=int, default=1920, help="Color width")
parser.add_argument('--color_height', type=int, default=1080, help="Color height")
parser.add_argument('--depth_width' , type=int, default=640,  help="Depth width")
parser.add_argument('--depth_height', type=int, default=480,  help="Depth height")
parser.add_argument('--frames',       type=int, default=200,  help="Number of frames for playback")

#parse arguments
args = parser.parse_args(sys.argv[1:])

context = streaming.context()
print context.get_device_count()
device = context.get_device(0)
print device.get_name()
device.enable_stream(streaming.stream_depth, args.depth_width, args.depth_height, streaming.format_z16, 30)
device.enable_stream(streaming.stream_color, args.color_width, args.color_height, streaming.format_rgb8, 30)
device.start()

for frame in range(0, args.frames):

        device.wait_for_frames()
        frame_data = device.get_frame_data(streaming.stream_color)
        #Get data from pointer
        data = streaming.cdata(frame_data, args.color_width*args.color_height*3) # 3 is a number of bytes in rgb8 format
        #Save image as a output
        img = Image.frombuffer('RGB', (args.color_width,args.color_height), data)
        img.save("streaming_color_"+str(frame)+".jpg")

device.stop()
