#include <iostream>
#include <cxcore.h>
#include <highgui.h>

int main(int argc, char* argv[]) {
	CvCapture *camera;

	camera = cvCreateCameraCapture(-1);

	IplImage *frame = 0;

	cvNamedWindow("Camera (:", 1);

	while(cvGetWindowHandle("Camera (:")) {
		frame = cvQueryFrame(camera);

		cvSaveImage("webcam.jpg", &frame);
		cvShowImage("Camera (:", &frame);

		int keyPressed = cvWaitKey(2);

		if(keyPressed == 27)
			break;
	}
	
	cvReleaseCapture(&camera);
	return 0;
}
