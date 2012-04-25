#include <highgui.h>
#include <cv.h>
#include <stdio.h>

int apply_value = 0;

int gaussian_value, gaussian_max;

int canny_enable, canny_low_value, canny_high_value, canny_low_max, canny_high_max;

int hough_enable, hough_minDist_value, hough_minDist_max, hough_param1_value, hough_param1_max, hough_param2_value, hough_param2_max, hough_minRadius_value, hough_minRadius_max, hough_maxRadius_value, hough_maxRadius_max;

int hue_min_value, hue_max, sat_min_value, sat_max, val_min_value, val_max, hue_max_value, sat_max_value, val_max_value;

IplImage *src_org, *src ,*dst;

void init_values(){

	//gaussian
	gaussian_value = 1;
	gaussian_max = 50;


	//canny
	canny_enable = 0;

	canny_low_value = 1;
	canny_high_value = 70;

	canny_low_max = 300;
	canny_high_max = 300;


	//hough
	hough_enable = 0;

	hough_minDist_value = 30;
	hough_param1_value = 100;
	hough_param2_value = 40;
	hough_minRadius_value = 10;
	hough_maxRadius_value = 100;

	hough_minDist_max = 200;
	hough_param1_max = 200;
	hough_param2_max = 200;
	hough_minRadius_max = 100;
	hough_maxRadius_max = 300;


	//hsv
	hue_min_value = 0;
	sat_min_value = 200;
	val_min_value = 200;

	hue_max_value = 40;
	sat_max_value = 255;
	val_max_value = 255;

	hue_max = 360;
	sat_max = 255;
	val_max = 255;

}


void updateSrc(){
	cvShowImage("src",src);
}

void applySrcCallBack(int pos){
	if( pos ){
        	cvCopy(dst,src);
        	updateSrc();
	}
}

void clearDst(){
	if ( dst ) { cvReleaseImage(&dst); }
	dst = cvCreateImage(cvGetSize(src),src->depth,src->nChannels);
}

void showDst(){
	cvShowImage("dst",dst);
}

IplImage* convertRGB2HSV(IplImage * src){
	if ( src->nChannels == 3 ){
	
		IplImage *hsv = cvCreateImage(cvGetSize(src),src->depth,src->nChannels);
	
		cvCvtColor(src,hsv,CV_BGR2HSV);

		return hsv;
	}
	
	return 0;
}

void mouseCallBack(int event, int x, int y, int flags, void *param){
        if (event == CV_EVENT_LBUTTONDOWN){

                IplImage* img = (IplImage*)(param);
		IplImage* hsv;		

		hsv = convertRGB2HSV(img);

		if( hsv ){
			
			CvScalar s = cvGet2D(hsv,y,x);

			printf("H:%f S:%f V:%f\n",s.val[0],s.val[1],s.val[2]);

			cvReleaseImage(&hsv);
		} 			
	} 
}

void houghCallBack(int pos){
	if ( pos ){
		
		IplImage *hsv = convertRGB2HSV(src);

		if ( hsv ){

			IplImage *thresholded = cvCreateImage(cvGetSize(src),src->depth,1);

			CvScalar hsv_min = cvScalar(hue_min_value, sat_min_value, val_min_value);
	
			CvScalar hsv_max = cvScalar(hue_max_value, sat_max_value, val_max_value);

			cvInRangeS(hsv,hsv_min,hsv_max,thresholded);

			//cvSaveImage("thresholded.jpg",thresholded);

			CvMemStorage *storage = cvCreateMemStorage(0);

			CvSeq* circles = cvHoughCircles(thresholded,storage,CV_HOUGH_GRADIENT,2,hough_minDist_value,hough_param1_value,hough_param2_value,hough_minRadius_value,hough_maxRadius_value);
			
			printf("found %d circles\n",circles->total);

			int i;

			for ( i=0; i<circles->total; i++ ){
				float *p = (float*)cvGetSeqElem(circles,i);
				printf("\tcircle%d x:%f y:%f r:%f\n",i,p[0],p[1],p[2]);
				cvCircle(hsv,cvPoint(cvRound(p[0]),cvRound(p[1])),cvRound(p[2]),CV_RGB(255,0,0),3,8,0);
			}
			
			cvNamedWindow("Hough");

                        cvShowImage("Hough",hsv);

			cvNamedWindow("thresholdedByHSV");

                        cvShowImage("thresholdedByHSV",thresholded);

			//cvSaveImage("result.jpg",src_backup);
			
			printf("enter some key to kill Hough result...\n");
			cvWaitKey();

			cvDestroyWindow("thresholdedByHSV");
			cvDestroyWindow("Hough");

			cvReleaseImage(&hsv);
			cvReleaseImage(&thresholded);
		}		
	}
}

void cannyCallBack(int pos){
	if( pos ){ 
		
		if ( src->nChannels == 3 ){

			IplImage *src_gray = cvCreateImage(cvGetSize(src),IPL_DEPTH_8U,1);	
	
			cvCvtColor(src,src_gray,CV_RGB2GRAY);
			
			cvReleaseImage(&src);

			src = cvCreateImage(cvGetSize(src_gray),src_gray->depth,src_gray->nChannels);

			cvCopy(src_gray,src);		

			cvReleaseImage(&src_gray);

		}

		printf("canny-> low:%d high: %d\n",canny_low_value,canny_high_value);

		clearDst();
		cvCanny(src,dst,canny_low_value,canny_high_value,3);
		showDst();
	}
}

void gaussianCallBack(int pos){
	if( pos%2 == 0 ) { pos+=1; }
	printf("gaussian-> :%d\n",pos);
	clearDst();
	cvSmooth(src,dst,CV_GAUSSIAN,pos,pos);
	showDst();
}

int main(int argc, char **argv){
	assert (argc == 2);

	init_values();
	
	src_org = cvLoadImage(argv[1]);
	
	//aloca memoria para origem "dinamica" para acontecer esta ser uma origem acumulativa!
	src = cvCreateImage(cvGetSize(src_org),src_org->depth,src_org->nChannels);
	
	cvCopy(src_org,src);

	//cvNamedWindow("src-org");
	//cvShowImage("src-org",src_org);

	cvNamedWindow("src");
	updateSrc();
	
	cvNamedWindow("dst");

	cvNamedWindow("config");

	cvNamedWindow("config2");
	
	int x = 30;
	int y = 50;
	int offset = 10;
	cvMoveWindow("src",x,y);
	cvMoveWindow("dst",x+offset+cvGetSize(src).width,y);
	cvMoveWindow("config",670,y);
	cvMoveWindow("config2",1000,y);

	cvCreateTrackbar("gaussian","config",&gaussian_value,gaussian_max,gaussianCallBack);
	cvCreateTrackbar("canny-low","config",&canny_low_value,canny_low_max,NULL);
	cvCreateTrackbar("canny-high","config",&canny_high_value,canny_high_max,NULL);
	cvCreateTrackbar("canny-en","config",&canny_enable,1,cannyCallBack);
	cvCreateTrackbar("hough-minDist","config",&hough_minDist_value,hough_minDist_max,NULL);
	cvCreateTrackbar("hough-param1","config",&hough_param1_value,hough_param1_max,NULL);
	cvCreateTrackbar("hough-param2","config",&hough_param2_value,hough_param2_max,NULL);
	cvCreateTrackbar("hough-minRadius","config",&hough_minRadius_value,hough_minRadius_max,NULL);
	cvCreateTrackbar("hough-maxRadius","config",&hough_maxRadius_value,hough_maxRadius_max,NULL);
	cvCreateTrackbar("hough-en","config",&hough_enable,1,houghCallBack);
	cvCreateTrackbar("apply","config",&apply_value,1,applySrcCallBack);	

	cvCreateTrackbar("hue-min","config2",&hue_min_value,hue_max,NULL);
	cvCreateTrackbar("hue-max","config2",&hue_max_value,hue_max,NULL);
	cvCreateTrackbar("sat-min","config2",&sat_min_value,sat_max,NULL);
	cvCreateTrackbar("sat-max","config2",&sat_max_value,sat_max,NULL);
	cvCreateTrackbar("val-min","config2",&val_min_value,val_max,NULL);
        cvCreateTrackbar("val-max","config2",&val_max_value,val_max,NULL);

	// HSV color picker na janela SRC
	cvSetMouseCallback("src",mouseCallBack,(void*)src);

	// loop central... ESQ finaliza!
	while(true){
		if(cvWaitKey(10) == 27) break;
	}
	
	cvDestroyWindow("src");
	//cvDestroyWindow("src-org");
	cvDestroyWindow("dst");
	cvDestroyWindow("config");
	cvDestroyWindow("config2");	
	
	cvReleaseImage(&src_org);
	cvReleaseImage(&src);
	cvReleaseImage(&dst);
}
