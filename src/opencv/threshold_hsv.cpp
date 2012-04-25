#include "cvaux.h"
#include "highgui.h"
#include "cxcore.h"
#include <stdio.h>

int main(int argc, char* argv[])
{

	assert (argc == 2);

	// recebe como parametro a imagem e atraves de uma faixa de valores do HSV, cria uma imagem de saida

	IplImage* frame = cvLoadImage(argv[1]);

	// utiliza Smooth para tirar o ruido da imagem
	// o valor dos parametros serao calibrados de 11, 11, ate 17,17 ja serao suficiente!
        cvSmooth(frame, frame, CV_GAUSSIAN, 11, 11);

	IplImage*  hsv_frame    = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 3);
	IplImage*  threshold1    = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 1);
	IplImage*  threshold2    = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 1);

	IplImage* threshold_sum = cvCreateImage(cvGetSize(frame),IPL_DEPTH_8U,1);

	CvScalar hsv_min = cvScalar(38, 50, 170, 0);
	CvScalar hsv_max = cvScalar(80, 100, 255, 0);
	CvScalar hsv_min2 = cvScalar(170, 50, 170, 0);
	CvScalar hsv_max2 = cvScalar(256, 180, 256, 0);

	// converte para HSV, sim o parametro eh CV_BGR2HSV e nao CV_RGB2HSV! 
	cvCvtColor(frame, hsv_frame, CV_BGR2HSV);

	cvInRangeS(hsv_frame, hsv_min, hsv_max, threshold1);

	cvInRangeS(hsv_frame, hsv_min2, hsv_max2, threshold2);

	// threshold_sum = threshold1 | threshold2
	cvOr(threshold1, threshold2, threshold_sum);
	
	cvNamedWindow("src",CV_WINDOW_AUTOSIZE);
	cvNamedWindow("threshold1",CV_WINDOW_AUTOSIZE);
	cvNamedWindow("threshold2",CV_WINDOW_AUTOSIZE);
	cvNamedWindow("threshold_sum",CV_WINDOW_AUTOSIZE);	
	cvShowImage("src",frame);
	cvShowImage("threshold1",threshold1);
	cvShowImage("threshold2",threshold2);
	cvShowImage("threshold_sum",threshold_sum);

	// espera por uma entrada
	cvWaitKey();

	cvDestroyAllWindows();

	cvReleaseImage(&frame);
	cvReleaseImage(&threshold1);
	cvReleaseImage(&threshold2);
	cvReleaseImage(&threshold_sum);
	
	return 0;
}
