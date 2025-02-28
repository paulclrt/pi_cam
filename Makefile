
capture_frame:
	g++ capture_frame.cpp -o capture_frame

live_camera:
	g++ live_camera.cpp -lSDL2 -o live_camera
