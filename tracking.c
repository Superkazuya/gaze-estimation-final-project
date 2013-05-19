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
  
/* THIS FUNCTION IS DIRTY. DO NOT USE
 *
int
camshift(const IplImage* current, const IplImage* next, RotatedBox* box)
  //Input: current previously found eye. next: whole search window.
{
  IplImage* img = cvCloneImage(next);
  IplImage* hsv_curr	= cvCreateImage(cvGetSize(current), IPL_DEPTH_8U, 3); //Size changes. No global or static
  IplImage* hsv_next	= cvCreateImage(cvGetSize(next), IPL_DEPTH_8U, 3); //Size changes. No global or static
  IplImage* h_curr	= cvCreateImage(cvGetSize(current), IPL_DEPTH_8U, 1);
  IplImage* h_next_8	= cvCreateImage(cvGetSize(next), IPL_DEPTH_8U, 1); //Size changes. No global or static
  IplImage* h_next	= cvCreateImage(cvGetSize(next), IPL_DEPTH_32F, 1);
  IplImage* img_bp	= cvCreateImage(cvGetSize(next), IPL_DEPTH_32F, 1);
  CvRect track_window;//   = cvRect(0, 0, next->width, next->height);

  //CvRect search_window	= CALC_RECT(CALC_RECT_CENTER_X(eye), CALC_RECT_CENTER_Y(eye), WINDOW_W, WINDOW_H);

  //Conversion
  cvCvtColor(current, hsv_curr, CV_BGR2HSV);
  cvCvtColor(next, hsv_next, CV_BGR2HSV);
  cvSplit(hsv_curr, h_curr, 0, 0, 0);
  cvSplit(hsv_next, h_next_8, 0, 0, 0);
  cvConvertScale(h_next_8, h_next, 1, 0);

  double max;
  cvCalcHist(&h_curr, hist, 0, NULL);
  cvNormalizeHist(hist, 1);
  cvCalcBackProject(&h_next, img_bp, hist);

  int moment0, s;
  //while(err > 2 && count < 15)
  {
    cvMinMaxLoc(img_bp, NULL, &max, NULL, NULL, NULL);
    moment0 = cvSum(img_bp).val[0];
    s = 2*sqrt(moment0/max);
    printf("s: %d \n", s);
    track_window = CALC_RECT(
    cvSetImageROI(img_bp, track_window);

  }
  printf("E:%d\n", moment0);
  int moment1_x, moment1_y, moment2_x, moment2_y, moment2_xy;
  IplImage* prob_x = cvCreateImage(cvSize(img_bp->width, 1), IPL_DEPTH_32F, 1);
  IplImage* val_x = cvCreateImage(cvSize(img_bp->width, 1), IPL_DEPTH_32F, 1);
  IplImage* prob_y = cvCreateImage(cvSize(1, img_bp->height), IPL_DEPTH_32F, 1);
  IplImage* val_y = cvCreateImage(cvSize(1, img_bp->height), IPL_DEPTH_32F, 1);
  IplImage* val_xy = cvCreateImage(cvGetSize(img_bp), IPL_DEPTH_32F, 1);
  cvReduce(img_bp, prob_x, 0, CV_REDUCE_SUM);
  cvReduce(img_bp, prob_y, 1, CV_REDUCE_SUM);
  int i, j;
  for (i = 0; i < val_x->width; i++) 
    cvSetReal2D(val_x, 0, i, i);
  for (i = 0; i < val_y->height; i++) 
    cvSetReal2D(val_y, i, 0, i);
  for (j = 0; j < val_y->height; j++) 
    for (i = 0; i < val_x->width; i++) 
      cvSetReal2D(val_xy, j, i, i*j);

  for (i = 0; i < val_x->width; i++) 
    printf("%f ", cvGetReal2D(prob_x, 0, i));

  moment1_x = cvDotProduct(val_x, prob_x);
  moment1_y = cvDotProduct(val_y, prob_y);

  cvPow(val_x, val_x, 2);
  cvPow(val_y, val_y, 2);
  moment2_x = cvDotProduct(val_x, prob_x);
  moment2_y = cvDotProduct(val_y, prob_y);
  moment2_xy = cvDotProduct(val_xy, img_bp);
  printf("%d %d\n", moment2_x, moment2_y);
  int xc = moment1_x/moment0;
  printf("xc:%d\n", xc);
  int yc = moment1_y/moment0;
  float a  = moment2_x/moment0-xc*xc;
  float b  = 2*(moment2_xy/moment0-xc*yc);
  float c  = moment2_y/moment0-yc*yc;
  box->center.x = xc;
  box->center.y = yc;
  box->size.height = sqrt((a+c+sqrt(b*b+(a-c)*(a-c)))/2);
  box->size.width  = sqrt((a+c-sqrt(b*b+(a-c)*(a-c)))/2);
  box->theta = atan((float)b/(a-c))/2;

  cvReleaseImage(&val_xy);
  cvReleaseImage(&val_y);
  cvReleaseImage(&prob_y);
  cvReleaseImage(&val_x);
  cvReleaseImage(&prob_x);

  draw(img, box);
  cvNamedWindow ("Tacgonl", CV_WINDOW_AUTOSIZE);
  cvShowImage	  ("Tacgonl", img_bp);
  cvWaitKey(0);
  cvNamedWindow ("Tacgonl", CV_WINDOW_AUTOSIZE);
  cvShowImage	  ("Tacgonl", img);
  cvWaitKey(0);

  cvReleaseImage(&img_bp);
  cvReleaseImage(&h_next);
  cvReleaseImage(&h_curr);
  cvReleaseImage(&hsv_next);
  cvReleaseImage(&hsv_curr);

  return(0);
}
*/
