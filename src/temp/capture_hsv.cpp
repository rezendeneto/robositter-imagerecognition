#include <highgui.h>
#include <cv.h>
#include <stdio.h>

void myCallBack(int event, int x, int y, int flags, void *param){
	if (event == CV_EVENT_LBUTTOWNDOWN){
		IplImage* img = (IplImage*)(param);
		CvScalar s = cvGet2D(hsv,y,x);
		printf("H:%f S:%f V:%f",s[0],s[1],s[2]);
	} 
}

int main(int argc, char **argv){
	assert(argc = =2);
	
	IplImage *src = cvLoadImage(argv[1]);

	plImage *hsv = createImage(cvGetSize(src),src->depth,src->nChannels);
	
	cvNamedWindow("hsv");

	cvCvtColor(src,hsv,CV_BGR2HSV);
	
	cvShowImage("hsv",hsv);

	cvSetMouseCallback("src",myCallBack,(void*)hsv);

	return 0;
}
