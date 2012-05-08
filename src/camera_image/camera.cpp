#include <stdlib.h>
#include <stdio.h>
#include <cv.h>
#include <highgui.h>

int _initialH = 20;
int _initialS = 100;
int _initialV = 100;

int _finalH = 30;
int _finalS = 255;
int _finalV = 255;

int tolerance = 25;

IplImage* GetThresholdedImage(IplImage* img) {
	// Converte a imagem em uma imagem com cores no padrão HSV
	IplImage* imgHSV = cvCreateImage(cvGetSize(img), 8, 3);
	cvCvtColor(img, imgHSV, CV_BGR2HSV);

	// Cria uma imagem de threshold a partir de img
	IplImage* imgThreshed = cvCreateImage(cvGetSize(img), 8, 1);

	// Valores de 20,100,100 até 30,255,255 estão funcionando perfeitamente para o amarelo em torno das 6h
	cvInRangeS(imgHSV, cvScalar(_initialH, _initialS, _initialV), cvScalar(_finalH, _finalS, _finalV), imgThreshed);

	// Libera memória para o objeto imgHSV
	cvReleaseImage(&imgHSV);

	return imgThreshed;
}

int main(int argc, char* argv[]) {
	// Inicializa as duas janelas a serem utilizadas
    cvNamedWindow("video");
	//cvNamedWindow("thresh");
	
	// Initialize capturing live feed from the camera
	CvCapture* capture = 0;
	capture = cvCaptureFromCAM(1);	

	// This image holds the "scribble" data...
	// the tracked positions of the ball
	//IplImage* imgScribble = NULL;
	
	//int i = 0;

	while(true) {
    	// Pega a figura que foi passada como argumento
    	//printf("Abrindo o arquivo %s...\n", argv[1]);
		IplImage* frame = cvQueryFrame(capture);
		/*char filename[10];
		sprintf(filename, "../../%d.jpeg", i, 10);
		printf("arquivo %s\n", filename);
		IplImage *frame = cvLoadImage(filename, 1);
		if(i < 10) i++;
		else i = 0;*/

		// Se não conseguir abrir a imagem, então volta ao loop
        if(!frame)
            continue;
		
		// If this is the first frame, we need to initialize it
		//if(imgScribble == NULL)
		//	imgScribble = cvCreateImage(cvGetSize(frame), 8, 3);

		// Holds the yellow thresholded image (yellow = white, rest = black)
		IplImage* imgYellowThresh = GetThresholdedImage(frame);

		// Calculate the moments to estimate the position of the ball
		CvMoments *moments = (CvMoments*)malloc(sizeof(CvMoments));
		cvMoments(imgYellowThresh, moments, 1);

		// The actual moment values
		double moment10 = cvGetSpatialMoment(moments, 1, 0);
		double moment01 = cvGetSpatialMoment(moments, 0, 1);
		double area = cvGetCentralMoment(moments, 0, 0);

		// Holding the last and current ball positions
		static int posX = 0;
		static int posY = 0;

		int lastX = posX;
		int lastY = posY;

		posX = moment10/area;
		posY = moment01/area;

		// Print it out for debugging purposes
		//printf("position (%d,%d)\n", posX, posY);

		// We want to draw a line only if its a valid position
		//if(lastX > 0 && lastY > 0 && posX > 0 && posY > 0)
			//cvLine(imgScribble, cvPoint(posX, posY), cvPoint(lastX, lastY), CV_RGB(255,0,0), 5);
		
		int minx=10000, miny=10000;
		int maxx=0, maxy=0;
	
		for (int h = 0; h < imgYellowThresh->height; h++) {
			for (int w = 0; w < imgYellowThresh->width; w++) {
				CvScalar currentColor = cvGet2D(imgYellowThresh,h,w);
				if(currentColor.val[0] > 0 || currentColor.val[1] > 0 || currentColor.val[2] > 0) {
					if (w > maxx) 
						maxx = w; 
					if (h > maxy) 
						maxy = h;
					if (w < minx) 
						minx = w; 
					if (h < miny) 
						miny = h;
				}
			}
		}

		cvRectangle(frame, cvPoint(minx,miny), cvPoint(maxx,maxy), CV_RGB(255,0,0), 3, 0, 0);
		
		//cvAdd(frame, imgScribble, frame);
		//cvShowImage("thresh", imgYellowThresh);
		cvShowImage("video", frame);

		// Wait for a keypress
		int c = cvWaitKey(10);
		if(c!=-1)
            break;
            
		cvReleaseImage(&imgYellowThresh);

		delete moments;
    }

    return 0;
}
