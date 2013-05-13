#include "isophote.h"
#include "config.h"

//static CvMat *filterx, *filtery, *filterxx, *filteryy;

void 
debug_print_singlechannel(const IplImage* img)
{
  int i, j;
  CvScalar tmp;
  for(i=0;i<img->height;i++)
  {
    for(j=0;j<img->width;j++)
      printf("%d ", (int)cvGetReal2D(img, i, j));
    printf("\n");
  }
}

void
display_image(const IplImage* img)
{
  double min, max;
  cvMinMaxLoc(img, &min, &max, NULL, NULL, NULL);
  printf("min %f, max %f\n", min, max);
  //max /= 2;
  IplImage* image = cvCreateImage(cvGetSize(img), IPL_DEPTH_8U, img->nChannels);
  cvConvertScaleAbs(img, image, 255/(max-min), -255*min/(max-min));

  cvNamedWindow ("Tacgonl", CV_WINDOW_AUTOSIZE);
  cvShowImage	  ("Tacgonl", image);
  cvWaitKey(0);
  cvReleaseImage(&image);
}

/*
void
isophote_init()
{
  int row, col;
  float p[] =  { 0.004711, 0.069321, 0.245410, 0.361117, 0.245410, 0.069321, 0.004711};
  float d1[] = { 0.018708, 0.125376, 0.193091, 0.00000, -0.193091,-0.125376,-0.018708};
  float d2[] = { 0.055336, 0.13777, -0.056554,-0.273118,-0.056554, 0.137778, 0.055336};
  filterx = cvCreateMat(7, 7, CV_32FC1);
  filtery = cvCreateMat(7, 7, CV_32FC1);
  filterxx = cvCreateMat(7, 7, CV_32FC1);
  filteryy = cvCreateMat(7, 7, CV_32FC1);
  for(row = 0; row < 7; row++)
    for(col = 0; col < 7; col++)
    {
      cvSetReal2D(filterx, row, col, d1[col]*p[row]);
      cvSetReal2D(filtery, row, col, d1[row]*p[col]);
      cvSetReal2D(filterxx, row, col, d2[col]*p[row]);
      cvSetReal2D(filteryy, row, col, d2[row]*p[col]);
    }
  printf("isophote calculation initialized.\n");
}
*/

void
calc_centermap(const IplImage* image, CvRect* window)
{
  assert(image->nChannels == 3);
  IplImage* eye	    = cvCreateImage(cvGetSize(image), image->depth, 1);
  cvCvtColor(image, eye, CV_RGB2GRAY);
  cvThreshold(eye, eye, COLOR_THRESHOLD, 255, CV_THRESH_TRUNC);
  //display_image(eye);
  IplImage* grad_x  = cvCreateImage(cvGetSize(image), IPL_DEPTH_16S, 1);
  IplImage* grad_y  = cvCreateImage(cvGetSize(image), IPL_DEPTH_16S, 1);
  IplImage* grad_xy  = cvCreateImage(cvGetSize(image), IPL_DEPTH_16S, 1);
  IplImage* grad_xx  = cvCreateImage(cvGetSize(image), IPL_DEPTH_16S, 1);
  IplImage* grad_yy  = cvCreateImage(cvGetSize(image), IPL_DEPTH_16S, 1);
  IplImage* coeff   = cvCreateImage(cvGetSize(image), IPL_DEPTH_32F, 1);
  //IplImage* img	    = cvCreateImage(cvGetSize(image), IPL_DEPTH_32F, 1);
  // todo: optimize

  cvSmooth(eye, eye, CV_GAUSSIAN, 1, 0, 0, 0);
  /*
  cvFilter2D(eye, grad_x, filterx, cvPoint(-1, -1));
  cvFilter2D(eye, grad_y, filtery, cvPoint(-1, -1));
  cvFilter2D(eye, grad_xx, filterxx, cvPoint(-1, -1));
  cvFilter2D(eye, grad_yy, filteryy, cvPoint(-1, -1));
  cvFilter2D(grad_x, grad_xy, filtery, cvPoint(-1, -1));
  */
  cvSobel(eye, grad_x, 1, 0, 5);
  cvSobel(eye, grad_y, 0, 1, 5);
  cvSobel(eye, grad_xx, 2, 0, 5);
  cvSobel(eye, grad_yy, 0, 2, 5);
  cvSobel(eye, grad_xy, 1, 1, 5);
  

  cvSetZero(coeff); //Use grad_xy as accumulator
  //cvSetZero(test); //Use grad_xy as accumulator
  int col, row;
  float* accumulator;
  float dx, dy, x, y, gx, gy, gxx, gyy, gxy;
  float coe, coedenum, norm;
  for(row = 0; row < eye->height; row++)
    for(col = 0; col < eye->width; col++)
    {
      gx = cvGetReal2D(grad_x, row, col);
      gy = cvGetReal2D(grad_y, row, col);
      gxx = cvGetReal2D(grad_xx, row, col)/2;
      gyy = cvGetReal2D(grad_yy, row, col)/2;
      gxy = cvGetReal2D(grad_xy, row, col)/2;
      coedenum = -gx*gx*gyy-gy*gy*gxx+2*gxy*gx*gy;
      if(coedenum < 0)
      {
	norm = gx*gx+gy*gy;
	coe = norm/coedenum;
	dx = gx*coe;
	dy = gy*coe;
	x = dx+col;
	y = dy+row;
      if(x < eye->width && 
	  y < eye->height &&
	   x >= 0 && 
	    y >= 0 &&
	     (dx || dy ) //Displacement != 0
	     )
	if(norm 
	    //&&(dx*dx + dy*dy) <= MAX_R2
	    //&&(dx*dx + dy*dy) <= MIN_R2
	    )
	{
	  accumulator = (float*)cvPtr2D(coeff, y, x, NULL);
	  (*accumulator) += sqrt(gx*gx+2*gxy*gxy+gy*gy);
	}
      }
    }
  cvSmooth(coeff, coeff, CV_GAUSSIAN, 9, 0, 0, 0);
  //display_image(test);
  //debug_print_singlechannel(test);

  //printf("interation: %d\n", meanshift(coeff, window));
  CvPoint max_loc;
  cvMinMaxLoc(coeff, NULL, NULL, NULL, &max_loc, NULL);
  //window->x = cvGetSize(coeff).width/2;
  //window->y = cvGetSize(coeff).height/2;
  window->x = max_loc.x;
  window->y = max_loc.y;
  window->width = PUPIL_SIZE/2;
  window->height = PUPIL_SIZE/2;


  CvConnectedComp track_comp;
  //printf("Iteration: %d\n", 
  cvMeanShift(coeff, *window, 
	  cvTermCriteria( CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, ITERATION_THRESHOLD, EPSILON), 
	  &track_comp);
  //);
  *window = track_comp.rect;
  /*
  cvRectangle(coeff, cvPoint(window->x, window->y), 
      cvPoint(window->x+window->width, window->y+window->height),
      CV_RGB(255, 255, 255), 1, 8, 0);

  */
  //display_image(coeff);

  cvReleaseImage(&coeff);
  cvReleaseImage(&grad_yy);
  cvReleaseImage(&grad_xx);
  cvReleaseImage(&grad_xy);
  cvReleaseImage(&grad_y);
  cvReleaseImage(&grad_x);
  cvReleaseImage(&eye);
}
