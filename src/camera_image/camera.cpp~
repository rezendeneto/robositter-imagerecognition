#include <stdlib.h>
#include <stdio.h>
#include <cv.h>
#include <highgui.h>

#define MAX_DISTANCE 200
#define MIN_DISTANCE 150
#define FRAMES 7
#define TOLERANCE 5
#define DIST_FROM_CENTER 40

#define ROTATE_LEFT 1
#define NO_ROTATE 2
#define ROTATE_RIGHT 3

#define WALK_UP 1
#define STAY 2
#define MOVE_AWAY 3

using namespace std;

int _lastRotationMovement = 0;
int _lastTranslationMovement = 0;

int _initialH = 46;
int _initialS = 38;
int _initialV = 67;

int _finalH = 95;
int _finalS = 255;
int _finalV = 255;

int _erodePos = 0;

int tolerance = 25;

void initialHueCallback(int pos) {
	_initialH = pos;
}

void initialSaturationCallback(int pos) {
	_initialS = pos;
}

void initialValueCallback(int pos) {
	_initialV = pos;
}

void finalHueCallback(int pos) {
	_finalH = pos;
}

void finalSaturationCallback(int pos) {
	_finalS = pos;
}

void finalValueCallback(int pos) {
	_finalV = pos;
}

void erodePosCallback(int pos) {
	_finalV = pos;
}

bool isBetweenTolerance(int value, double average) {
	
	return value > average - TOLERANCE && value < average + TOLERANCE;
}

int getDistanceFromPixels(int x) {
	double a0 = 0.000134352, a1 = 28.31883627, a2 = -0.887852876, a3 = 0.011472967, a4 = -6.8458e-05, a5 = 1.55387e-7;
	return a5 * pow(x, 5) + a4 * pow(x, 4) + a3 * pow(x, 3) + a2 * pow(x, 2) + a1 * x + a0;
}

void onMouseClicked(int event, int x, int y, int flags, void *param){
	if (event == CV_EVENT_LBUTTONDOWN){
		IplImage* img = (IplImage*)(param);
		CvScalar s = cvGet2D(img,y,x);
		printf("H:%f S:%f V:%f\n",s.val[0],s.val[1],s.val[2]);
	} 
}

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
	cvNamedWindow("thresh");
	cvNamedWindow("config");
		
	cvCreateTrackbar("initialH", "config", &_initialH, 255, initialHueCallback);
	cvCreateTrackbar("initialS", "config", &_initialS, 255, initialSaturationCallback);
	cvCreateTrackbar("initialV", "config", &_initialV, 255, initialValueCallback);
	
	cvCreateTrackbar("finalH", "config", &_finalH, 255, finalHueCallback);
	cvCreateTrackbar("finalS", "config", &_finalS, 255, finalSaturationCallback);
	cvCreateTrackbar("finalV", "config", &_finalV, 255, finalValueCallback);

	cvCreateTrackbar("erodePos", "config", &_erodePos, 100, erodePosCallback);
	
	// Initialize capturing live feed from the camera
	CvCapture* capture = 0;
	capture = cvCaptureFromCAM(1);	

	// This image holds the "scribble" data...
	// the tracked positions of the ball
	//IplImage* imgScribble = NULL;
	
	int i = 1;
	int widthSum = 0;
	int heightSum = 0;
	int objCentralXSum = 0;
	
	bool processFrame;

	while(true) {
	
		processFrame = i % FRAMES == 0;
	
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
		IplImage* imgThresh = GetThresholdedImage(frame);

		// Calculate the moments to estimate the position of the ball
		CvMoments *moments = (CvMoments*)malloc(sizeof(CvMoments));
		cvMoments(imgThresh, moments, 1);

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
		
		cvErode(imgThresh, imgThresh, NULL, 2);
		
		int minx=10000, miny=10000;
		int maxx=-10, maxy=-10;
	
		for (int h = 0; h < imgThresh->height; h++) {
			for (int w = 0; w < imgThresh->width; w++) {
				CvScalar currentColor = cvGet2D(imgThresh,h,w);
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
		cvLine(frame, cvPoint(frame->width/2, 0), cvPoint(frame->width/2, frame->height), cvScalar(255,0,0), 1);

		int width = maxx - minx;
		int height = maxy - miny;
		
		int objCentralX = minx + width/2;
		int objCentralY = miny + height/2;
		
		int distance = getDistanceFromPixels(width);
		
		widthSum += width;
		heightSum += height;
		objCentralXSum += objCentralX;
		
		int widthAvg = widthSum/FRAMES;
		int objCentralXAvg = objCentralXSum/FRAMES;
		
		int imgCentralX = frame->width/2;
		
		/*cout << "processFrame: " << processFrame << endl;
		cout << "widthAvg: " << widthAvg << endl;
		cout << "objCentralXAvg: " << objCentralXAvg << endl;
		cout << "width: " << width << endl;
		cout << "objCentralX: " << objCentralX << endl;
		cout << "Distância: " << distance << "cm" << endl;
		cout << "Ponto central: (" << objCentralX << ", " << objCentralY << ")" << endl;
		cout << "i: " << i << endl;*/
		
		int translationMovement = 0;
		int rotationMovement = 0;

		if(processFrame && isBetweenTolerance(width, widthAvg) && isBetweenTolerance(objCentralX, objCentralXAvg)) {
			if(minx != 10000 && miny != 10000 && maxx != -10 && maxy != -10) {
				// Define os movimentos de translação de acordo com a distância do alvo
				if(distance < MIN_DISTANCE) {
					translationMovement = MOVE_AWAY;
				}
				else if(distance > MAX_DISTANCE) {
					translationMovement = WALK_UP;
				}
				else {
					translationMovement = STAY;
				}
				
				// Define os movimentos de rotação de acordo com a distância do alvo
				if(objCentralX < imgCentralX - DIST_FROM_CENTER) {
					rotationMovement = ROTATE_LEFT;
				}
				else if(objCentralX > imgCentralX + DIST_FROM_CENTER) {
					rotationMovement = ROTATE_RIGHT;
				}
				else if(objCentralX > imgCentralX - DIST_FROM_CENTER && objCentralX < imgCentralX + DIST_FROM_CENTER) {
					rotationMovement = NO_ROTATE;
				}
			}
			else {
				rotationMovement = ROTATE_LEFT;
			}
			
			if(rotationMovement != _lastRotationMovement) {
				switch(rotationMovement) {
					case ROTATE_LEFT:
						cout << "Rotacione à esquerda" << endl;
					break;
					case NO_ROTATE:
						cout << "Pare de rotacionar" << endl;
					break;
					case ROTATE_RIGHT:
						cout << "Rotacione à direita" << endl;
					break;
				}
			}
			
			if(translationMovement != _lastTranslationMovement) {
				switch(translationMovement) {
					case MOVE_AWAY:
						cout << "Afaste" << endl;
					break;
					case STAY:
						cout << "Pare" << endl;
					break;
					case WALK_UP:
						cout << "Avance" << endl;
					break;
				}
			}
			
			_lastRotationMovement = rotationMovement;
			_lastTranslationMovement = translationMovement;
		}

		//cvAdd(frame, imgScribble, frame);
		cvShowImage("thresh", imgThresh);
		cvShowImage("video", frame);
		
		cvSetMouseCallback("video", onMouseClicked, (void*)frame);
		cvSetMouseCallback("thresh", onMouseClicked, (void*)imgThresh);

		// Wait for a keypress
		int c = cvWaitKey(10);
		if(c!=-1)
            break;
            
		cvReleaseImage(&imgThresh);

		delete moments;
		
		// Zera as somas utilizadas para calcularem as médias
		if(processFrame)
			objCentralXSum = widthSum = heightSum = 0;
		
		i++;
    }

    return 0;
}
