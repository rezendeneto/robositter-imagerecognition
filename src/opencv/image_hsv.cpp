#include <highgui.h>
#include <cv.h>
#include <stdio.h>

// teste!

void myCallBack(int event, int x, int y, int flags, void *param){
	if (event == CV_EVENT_LBUTTONDOWN){
		IplImage* img = (IplImage*)(param);
		CvScalar s = cvGet2D(img,y,x);
		printf("H:%f S:%f V:%f\n",s.val[0],s.val[1],s.val[2]);
	} 
}

int main(int argc, char **argv){
	assert(argc == 2);

	// recebe como argumento a imagem a ser convertida em hsv e exibe em uma janela e atraves dos clicks do mouse exibe o valor do HSV.
	
	IplImage *src = cvLoadImage(argv[1]);

	IplImage *hsv = cvCreateImage(cvGetSize(src),src->depth,src->nChannels);

	cvNamedWindow("src");	
	cvNamedWindow("hsv");

	cvCvtColor(src,hsv,CV_BGR2HSV);
	
	cvShowImage("hsv",hsv);
	cvShowImage("src",src);

	cvSetMouseCallback("hsv",myCallBack,(void*)hsv);
	
	// ESC finaliza

	while(true){
		if(cvWaitKey() == 27) break;
	}
	
	cvReleaseImage(&src);
	cvReleaseImage(&hsv);
	cvDestroyAllWindows();
	
	return 0;
}
