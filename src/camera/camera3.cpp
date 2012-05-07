#include "cv.h" 
 #include "highgui.h" 
 #include <stdio.h>  
 // A Simple Camera Capture Framework 
 int main() {
VideoCapture capture(0);
if(!capture.isOpened())
{
    cout << "ERROR: VideoCapture wasn't opened" << endl;
    //return -1;
}
capture.set(CV_CAP_PROP_FRAME_WIDTH, CAM_WIDTH);
capture.set(CV_CAP_PROP_FRAME_HEIGHT, CAM_HEIGHT);

cout << "WIDTH: " << capture.get(CV_CAP_PROP_FRAME_WIDTH) << endl;
cout << "HEIGHT: " << capture.get(CV_CAP_PROP_FRAME_HEIGHT) << endl;

if(!capture.isOpened())
    cout << "it was not opened!" << endl;
Mat mRGBcam;//(CAM_HEIGHT, CAM_WIDTH, CV_8UC3);
cout << "capturing ..." << endl;
capture >> mRGBcam;
cout << "CAPTURE DONE!" << endl;

 }
