import streaming
import projection
import image
from PIL import Image     

a = streaming.context()
print a.get_device_count()
d = a.get_device(0)
print d.get_name()
depth_width = 628
depth_height = 468
color_width = 1920
color_height = 1080
d.enable_stream(streaming.stream_depth, depth_width, depth_height, streaming.format_z16, 30)
d.enable_stream(streaming.stream_color, color_width, color_height, streaming.format_rgb8, 30)
d.start()

color_intrin =  d.get_stream_intrinsics(streaming.stream_color)
depth_intrin =  d.get_stream_intrinsics(streaming.stream_depth)
extrin =        d.get_extrinsics(streaming.stream_depth, streaming.stream_color)

p = projection.projection_interface_create_instance(projection.convert_intrinsics(color_intrin), projection.convert_intrinsics(depth_intrin), projection.convert_extrinsics(extrin))

for x in range(0, 5):
	#print d.is_streaming()
	d.wait_for_frames()

        frame_data_color = d.get_frame_data(streaming.stream_color)
        frame_data_depth = d.get_frame_data(streaming.stream_depth)

        ColorInfo = image.image_info();
        ColorInfo.width = color_width;
        ColorInfo.height = color_height;
        ColorInfo.format = image.pixel_format_rgb8 #projection.convert_pixel_format(streaming.format_rgb8);
        ColorInfo.pitch = 4 * color_width;
        image_data_color = image.image_data_with_data_releaser(frame_data_color);
        colorImage = image.image_interface_create_instance_from_raw_data( ColorInfo,
                                                                               image_data_color,
                                                                               image.stream_type_color,
                                                                               image.image_interface.any,
                                                                               d.get_frame_timestamp(streaming.stream_color),
                                                                               d.get_frame_number(streaming.stream_color));

        DepthInfo = image.image_info();
        DepthInfo.width = depth_width;
        DepthInfo.height = depth_height;
        DepthInfo.format = image.pixel_format_z16 #projection.convert_pixel_format(streaming.format_z16);
        DepthInfo.pitch = 2 * depth_width;
        image_data_depth = image.image_data_with_data_releaser(frame_data_depth);
        depthImage = image.image_interface_create_instance_from_raw_data( DepthInfo,
                                                                               image_data_depth,
                                                                               image.stream_type_depth,
                                                                               image.image_interface.any,
                                                                               d.get_frame_timestamp(streaming.stream_depth),
                                                                               d.get_frame_number(streaming.stream_depth));

        uv_ret = projection.new_pointF32Array(DepthInfo.width * DepthInfo.height);
        p.query_uvmap(depthImage, uv_ret);

        data_depths = streaming.cdata(frame_data_depth, 5000)

        #for pixel in range(0, DepthInfo.width * DepthInfo.height):
               # val = projection.pointF32Array_getitem(uv_ret, pixel)
                #if(val.x >0 or val.y >0):
                    #print "frame - " + str(x) + " "+ str(pixel) + ": " + str(val.x) + "  " + str(val.y)

        data_col = streaming.cdata(frame_data_color, color_width*color_height*3) #640*480*4
        data_col_uchar = projection.void_to_char(frame_data_color)
        uvmap_data= ""


        test_data= ""
        for v in range (color_height-1, -1, -1):
        #print str(u)
            for u in range(0, color_width, 1):
                pos = u+v*color_width;
                pos3 = pos*3
                test_data += chr(projection.ucharArray_getitem(data_col_uchar, pos3 + 0)) #data_col[pos3 + 0] #chr(255);
                test_data += chr(projection.ucharArray_getitem(data_col_uchar, pos3 + 1)) #data_col[pos3 + 1] #chr(255);
                test_data += chr(projection.ucharArray_getitem(data_col_uchar, pos3 + 2)) #data_col[pos3 + 2] #chr(255);


        #Due to uvmap_data is string we should fill image array vise versa
        for v in range (depth_height-1, -1, -1):
            #print str(u)
            for u in range(0, depth_width, 1):

                #u = depth_width - u - 1;
                #v = depth_height - v - 1;
                pos = u+v*depth_width;
                pos3 = pos*3

                val = projection.pointF32Array_getitem(uv_ret, pos)
                i = int(val.x*color_width);
                j = int(val.y*color_height);

                if(int(i) < 0 or int(j) < 0):
                    uvmap_data += chr(0);
                    uvmap_data += chr(0);
                    uvmap_data += chr(255);
                    continue

                #print  str(u) + " " +   str(v)+ " => " +  str(i) + " " +   str(j)
                #uvmap_data += chr(255);
                #uvmap_data += chr(255);
                #uvmap_data += chr(255);
                #print projection.ucharArray_getitem(data_col_uchar, pos3 + 0);
                pos_color = 3* (i + j * color_width);
                uvmap_data += chr(projection.ucharArray_getitem(data_col_uchar, pos_color + 0)) #data_col[pos3 + 0] #chr(255);
                uvmap_data += chr(projection.ucharArray_getitem(data_col_uchar, pos_color + 1)) #data_col[pos3 + 1] #chr(255);
                uvmap_data += chr(projection.ucharArray_getitem(data_col_uchar, pos_color + 2)) #data_col[pos3 + 2] #chr(255);


                #print str(ord(data_depths[2*pixel])) + "  " + str(ord(data_depths[2*pixel + 1]))
        #save color data to file

        img = Image.frombuffer('RGB', (color_width,color_height), data_col)
        img.save("a_test_"+str(x)+".jpg")

        img = Image.frombuffer('RGB', (color_width,color_height), data_col)
        img.save("a_TEST_"+str(x)+".jpg")
	
        img1 = Image.frombuffer('RGB', (depth_width,depth_height), uvmap_data)
        img1.save("a_test_uv_"+str(x)+".jpg")

        #data_depths = streaming.cdata(frame_data_depth, 500) #640*480*4
	#for i in range(0, 400):
	#	print ord(data[i])

d.stop()
