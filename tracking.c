#include "tracking.h"
#include <stdio.h>
#include <math.h>

#define MAX_ITER 15
#define EPSILON  0.1
#define ABOVE_ZERO(X) (X > 0 ? X : 0)
#define CALC_RECT(X, Y, W, H) cvRect(ABOVE_ZERO(X-(W-1)/2), ABOVE_ZERO(Y-(H-1)/2), W, H)
#define CALC_RECT_CENTER_X(X) (X->x-(X->width-1)/2) 
#define CALC_RECT_CENTER_Y(X) (X->y-(X->height-1)/2) 

void
camshift_init(const IplImage* img_template, TrackObject* obj)
{
  int size[] = {255};
  float  h_range[] = {0, 255};
  float* range[] = {h_range};
  obj->hist = cvCreateHist(1,  size, CV_HIST_ARRAY, range, 1);
  IplImage* hsv=  cvCreateImage(cvGetSize(img_template), IPL_DEPTH_8U, 3); //Size changes. No global or static
  IplImage* hue	= cvCreateImage(cvGetSize(img_template), IPL_DEPTH_8U, 1);
  cvCvtColor(img_template, hsv, CV_BGR2HSV);
  cvSplit(hsv, hue, 0, 0, 0);
  cvCalcHist(&hue, obj->hist, 0, NULL);
  cvNormalizeHist(obj->hist, 1);

  obj->track_window= cvRect(0, 0, img_template->width, img_template->height);

  cvReleaseImage(&hue);
  cvReleaseImage(&hsv);
}

int
camshift(const IplImage* next, TrackObject* obj)
  //Input: next : Next Frame, obj : offline tracking object
{
  IplImage* img = cvCloneImage(next);
  IplImage* hsv_next	= cvCreateImage(cvGetSize(next), IPL_DEPTH_8U, 3); //Size changes. No global or static
  IplImage* h_next_8	= cvCreateImage(cvGetSize(next), IPL_DEPTH_8U, 1); //Size changes. No global or static
  IplImage* h_next	= cvCreateImage(cvGetSize(next), IPL_DEPTH_32F, 1);
  IplImage* img_bp	= cvCreateImage(cvGetSize(next), IPL_DEPTH_32F, 1);
  CvConnectedComp track_comp;

  //CvRect search_window	= CALC_RECT(CALC_RECT_CENTER_X(eye), CALC_RECT_CENTER_Y(eye), WINDOW_W, WINDOW_H);

  //Conversion
  cvCvtColor(next, hsv_next, CV_BGR2HSV);
  cvSplit(hsv_next, h_next_8, 0, 0, 0);
  cvConvertScale(h_next_8, h_next, 1, 0);

  cvCalcBackProject(&h_next, img_bp, obj->hist);

  int iteration;
  iteration = cvCamShift(img_bp, obj->track_window, 
	cvTermCriteria( CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, MAX_ITER, EPSILON), 
	&track_comp, &obj->track_box);
  obj->track_window = track_comp.rect;

  cvReleaseImage(&img_bp);
  cvReleaseImage(&h_next);
  cvReleaseImage(&h_next_8);
  cvReleaseImage(&hsv_next);
  cvReleaseImage(&img);

  return(iteration);
}


void
camshift_free(TrackObject* obj)
{
  if(obj->hist)
    cvReleaseHist(&(obj->hist));
}
