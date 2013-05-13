#pragma once
#include "cv.h"
#include "highgui.h"
#include <stdio.h>
#include <math.h>
#include <stdio.h>

extern void calc_centermap(const IplImage*, CvRect*);
extern void debug_print_singlechannel(const IplImage*);
//extern void calc_grad_x(const IplImage*, IplImage* );
//extern void calc_grad_y(const IplImage*, IplImage* );
extern void display_image(const IplImage*);
//extern int  meanshift(const IplImage* in, CvRect* window);
//extern void isophote_init();
//extern void isophote_destroy();

#define ABOVE_ZERO(X) (X > 0 ? X : 0)
#define CALC_RECT(X, Y, W, H) cvRect(ABOVE_ZERO(X-(W-1)/2), ABOVE_ZERO(Y-(H-1)/2), W, H)
#define CALC_POINT(X) cvPoint(X.x+X.width/2, X.y+X.height/2)
//#define CALC_POINT(X) cvPoint(X->x+X->width/2, X->y+X->height/2)
#define EPSILON 1
#define ITERATION_THRESHOLD 20
#define PUPIL_SIZE 6
#define MAX_R2 250
#define MIN_R2 100
