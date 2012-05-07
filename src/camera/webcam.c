 #ifdef _CH_
#pragma package <opencv>
#endif

#ifndef _EiC
#include "cv.h"
#include "highgui.h"
#include <stdio.h>
#include <ctype.h>
#endif

void showCaptureProperties(CvCapture* cap){
 int  w,h,f;
 w=(int)cvGetCaptureProperty(cap,CV_CAP_PROP_FRAME_WIDTH);
 h=(int)cvGetCaptureProperty(cap,CV_CAP_PROP_FRAME_HEIGHT);
 f=(int)cvGetCaptureProperty(cap,CV_CAP_PROP_FPS);
 printf("Capture properties (widthxheight - frames): %dx%d - %d\n",w,h,f);
}

int main( int argc, char** argv )
{
 CvCapture* capture = 0;

 if( argc == 1 || (argc == 2 && strlen(argv[1]) == 1 && isdigit(argv[1][0])))
 capture = cvCaptureFromCAM( argc == 2 ? argv[1][0] - '0' : 0 );
 else if( argc == 2 )
 capture = cvCaptureFromAVI( argv[1] );

 if( !capture )
 {
 fprintf(stderr,"Could not initialize capturing...\n");
 return -1;
 }

 printf( "Hot keys: \n"
 "\tESC - quit the program\n"
 "\tDavid MillÃ¡n EscrivÃ¡ | Damiles\n");

 cvNamedWindow( "CamShiftDemo", 1 );

 cvQueryFrame(capture);//Need to get correct data
 if(cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH,320))
 printf("Cant set width property\n");

 if(cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, 240))
 printf("Cant set height property\n");

 showCaptureProperties(capture);
 for(;;)
 {
 IplImage* frame = 0;
 int c;

 frame = cvQueryFrame( capture );
 if( !frame )
 break;

 cvShowImage( "CamShiftDemo", frame );

 c = cvWaitKey(10);
 if( (char) c == 27 )
 break;
 }

 cvReleaseCapture( &capture );
 cvDestroyWindow("CamShiftDemo");

 return 0;
}

#ifdef _EiC
main(1,"camdemo.c");
#endif


