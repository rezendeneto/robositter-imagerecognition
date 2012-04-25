#test-application.make

build:
	mkdir build
	g++ `pkg-config --cflags opencv` src/opencv_control/main.cpp -o build/main `pkg-config --libs opencv`
	g++ `pkg-config --cflags opencv` src/opencv/image_hsv.cpp -o build/image_hsv `pkg-config --libs opencv`
	g++ `pkg-config --cflags opencv` src/opencv/threshold_hsv.cpp -o build/threshold_hsv `pkg-config --libs opencv`
	
clean:
	rm -Rf build/
