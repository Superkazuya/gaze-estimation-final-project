#pragma once
#include "cv.h"
#include "highgui.h"
#include <stdio.h>
#include <math.h>
#include <stdio.h>

extern void calc_centermap(const IplImage*, CvRect*);
extern void calc_stable_ic(const IplImage*, CvRect*);
extern void debug_print_singlechannel(const IplImage*);
//extern void calc_grad_x(const IplImage*, IplImage* );
//extern void calc_grad_y(const IplImage*, IplImage* );
extern void display_image(const IplImage*);
//extern int  meanshift(const IplImage* in, CvRect* window);
//extern void isophote_init();
//extern void isophote_destroy();

