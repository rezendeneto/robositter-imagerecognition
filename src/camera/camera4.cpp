#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/time.h>

#include"cv.h"
#include"highgui.h"
#include"cxtypes.h"


#define ERROR -1
#define NO_ERROR 1


int main()
{
    int camera_index = 0;
    IplImage *color_frame = NULL;
    int exit_key_press = 0;

    CvCapture *capture = NULL;
    capture = cvCaptureFromCAM(camera_index);
    if (!capture)
    {
        printf("!!! ERROR: cvCaptureFromCAM\n");
        return -1;
    }

    cvNamedWindow("Grayscale video", CV_WINDOW_AUTOSIZE);

    while (exit_key_press != 'q')
    {
        /* Capture a frame */
        color_frame = cvQueryFrame(capture);
        if (color_frame == NULL)
        {
            printf("!!! ERROR: cvQueryFrame\n");
            break;
        }
        else
        {
            IplImage* gray_frame = cvCreateImage(cvSize(color_frame->width, color_frame->height), color_frame->depth, 1);  
            if (gray_frame == NULL)
            {
                printf("!!! ERROR: cvCreateImage\n");
                continue;
            }

            cvCvtColor(color_frame, gray_frame, CV_BGR2GRAY);
            cvShowImage("Grayscale video", gray_frame);
            cvReleaseImage(&gray_frame);
        }
            exit_key_press = cvWaitKey(1);
    }

    cvDestroyWindow("Grayscale video");
    cvReleaseCapture(&capture);

    return 0;
}
