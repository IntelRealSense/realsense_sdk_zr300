import playback
from PIL import Image     

a = playback.context("/tmp/a.rssdk")
print a.get_device_count()
d = a.get_device(0)
print d.get_name()
d.enable_stream(playback.stream_depth, 640, 480, playback.format_z16, 30)
d.enable_stream(playback.stream_color, 1920, 1080, playback.format_rgb8, 30)
d.start()

for x in range(0, 200):
	#print d.is_streaming()
	d.wait_for_frames()
	frame_data = d.get_frame_data(playback.stream_color)
	data = playback.cdata(frame_data, 1920*1080*3) #640*480*4
	img = Image.frombuffer('RGB', (1920,1080), data)
	img.save("a_test_"+str(x)+".jpg")
	
	#for i in range(0, 400):
	#	print ord(data[i])

d.stop()
