#pragma once
#include "cv.h"
#include "highgui.h"
#define HIST_DIM 1  //H Plane
#define SIZE	255
#define CAMSHIFT_MAX_ITER 15
#define CAMSHIFT_EPSILON 0.1
/*
static int	size = SIZE;
static float	range[HIST_DIM] = {0,360}; static CvConnectedComp track_comp;
static CvBox2D	track_box;
CvHistogram* hist = cvCreateHist(HIST_DIM, &size, CV_HIST_ARRAY, &range, 1); 

extern int	meanshift(const IplImage*, const IplImage*);
*/

typedef struct{
  CvHistogram* hist;
  CvBox2D track_box;
  CvRect track_window;
}TrackObject;

extern void camshift_init (const IplImage*, TrackObject*);
extern int  camshift	  (const IplImage*, TrackObject*); 
extern void camshift_free (TrackObject*);
