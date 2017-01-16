# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2016 Intel Corporation. All Rights Reservedevice.

import sys
import argparse
import streaming
import projection
from PIL import Image     


#setup arguments
parser = argparse.ArgumentParser()
parser.add_argument('--file', default="/tmp/a.rssdk", help="File for input. Depth (z16 format) and color (rgb8 format) stream should be appeared in file.")
parser.add_argument('--color_width' , type=int, default=1920, help="Color width")
parser.add_argument('--color_height', type=int, default=1080, help="Color height")
parser.add_argument('--depth_width' , type=int, default=628,  help="Depth width")
parser.add_argument('--depth_height', type=int, default=468,  help="Depth height")
parser.add_argument('--frames',       type=int, default=5,  help="Number of frames for playback")

#parse arguments
args = parser.parse_args(sys.argv[1:])

context = streaming.context()
print context.get_device_count()
device = context.get_device(0)
print device.get_name()

device.enable_stream(streaming.stream_depth, args.depth_width, args.depth_height, streaming.format_z16, 30)
device.enable_stream(streaming.stream_color, args.color_width, args.color_height, streaming.format_rgb8, 30)
device.start()

color_intrin =  device.get_stream_intrinsics(streaming.stream_color)
depth_intrin =  device.get_stream_intrinsics(streaming.stream_depth)
extrin =        device.get_extrinsics(streaming.stream_depth, streaming.stream_color)

p = projection.projection_interface_create_instance(projection.convert_intrinsics(color_intrin), projection.convert_intrinsics(depth_intrin), projection.convert_extrinsics(extrin))

for x in range(0, args.frames):

        device.wait_for_frames()

        frame_data_color = device.get_frame_data(streaming.stream_color)
        frame_data_depth = device.get_frame_data(streaming.stream_depth)

        ColorInfo = projection.image_info()
        ColorInfo.width = 1920 #args.color_width
        ColorInfo.height = args.color_height
        ColorInfo.format = projection.pixel_format_rgb8
        ColorInfo.pitch = 4 * args.color_width
        image_data_color = projection.image_data_with_data_releaser(frame_data_color);
        colorImage = projection.image_interface_create_instance_from_raw_data( ColorInfo,
                                                                               image_data_color,
                                                                               projection.stream_type_color,
                                                                               projection.image_interface.any,
                                                                               device.get_frame_timestamp(streaming.stream_color),
                                                                               device.get_frame_number(streaming.stream_color));

        DepthInfo = projection.image_info();
        DepthInfo.width = args.depth_width;
        DepthInfo.height = args.depth_height;
        DepthInfo.format = projection.pixel_format_z16
        DepthInfo.pitch = 2 * args.depth_width;
        image_data_depth = projection.image_data_with_data_releaser(frame_data_depth);
        depthImage = projection.image_interface_create_instance_from_raw_data( DepthInfo,
                                                                               image_data_depth,
                                                                               projection.stream_type_depth,
                                                                               projection.image_interface.any,
                                                                               device.get_frame_timestamp(streaming.stream_depth),
                                                                               device.get_frame_number(streaming.stream_depth));

        uv_ret = projection.new_pointF32Array(DepthInfo.width * DepthInfo.height);
        p.query_uvmap(depthImage, uv_ret);

        data_col = streaming.cdata(frame_data_color, args.color_width*args.color_height*3) #640*480*4
        data_col_uchar = projection.void_to_char(frame_data_color)
        uvmap_data= ""

        #if you want to have non-rotated image uncomment next line and commentout line after next line
        #for v in range (args.depth_height-1, -1, -1): #Due to uvmap_data is string we should fill image array vise versa
        for v in range (0, args.depth_height, 1):
            #print str(u)
            for u in range(0, args.depth_width, 1):


                pos = u+v*args.depth_width;
                pos3 = pos*3

                val = projection.pointF32Array_getitem(uv_ret, pos)
                i = int(val.x*args.color_width);
                j = int(val.y*args.color_height);

                if(int(i) < 0 or int(j) < 0):
                    uvmap_data += chr(0);
                    uvmap_data += chr(0);
                    uvmap_data += chr(255);
                    continue

                pos_color = 3* (i + j * args.color_width);
                uvmap_data += chr(projection.ucharArray_getitem(data_col_uchar, pos_color + 0)) #data_col[pos3 + 0] #chr(255);
                uvmap_data += chr(projection.ucharArray_getitem(data_col_uchar, pos_color + 1)) #data_col[pos3 + 1] #chr(255);
                uvmap_data += chr(projection.ucharArray_getitem(data_col_uchar, pos_color + 2)) #data_col[pos3 + 2] #chr(255);


        #save color data to file
        img = Image.frombuffer('RGB', (args.color_width,args.color_height), data_col)
        img.save("projection_color_"+str(x)+".jpg")

        #save uvmap to file
        img1 = Image.frombuffer('RGB', (args.depth_width,args.depth_height), uvmap_data)
        img1.save("projection_uvmap_"+str(x)+".jpg")


device.stop()
