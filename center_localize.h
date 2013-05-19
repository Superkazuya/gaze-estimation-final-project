#pragma once
#include "cv.h"
#include "highgui.h"
#include <stdio.h>
#include <math.h>

#define THRESHOLD_ALPHA 0.3
#define THRESHOLD_BETA 1

/*
extern void calc_weighted_gradient(const IplImage* img_eye, IplImage* grad);
extern void find_center_init(const IplImage* , IplImage* [H][W], CvSize);
extern CvPoint find_center(const IplImage* , const IplImage* [H][W], CvSize) ;
extern void debug_print_singlechannel(const IplImage* img);
extern CvPoint find_center(int i, int j, const IplImage* grad, CvSize size, const IplImage* cp_norm[H][W]);
*/
extern CvPoint calc_eyecenter(const IplImage*, CvRect);//, CvMat* filter_x, CvMat* filter_y);
extern CvPoint calc_heyecenter(const IplImage* );
