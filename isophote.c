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

void
calc_centermap(const IplImage* image, CvRect* window)
{
  IplImage* eye	    = cvCreateImage(cvGetSize(image), image->depth, 1);
  cvCopy(image, eye, NULL);
  //cvThreshold(eye, eye, COLOR_THRESHOLD, 255, CV_THRESH_TRUNC);
  //display_image(eye);
  IplImage* grad_x  = cvCreateImage(cvGetSize(image), IPL_DEPTH_16S, 1);
  IplImage* grad_y  = cvCreateImage(cvGetSize(image), IPL_DEPTH_16S, 1);
  IplImage* grad_xy  = cvCreateImage(cvGetSize(image), IPL_DEPTH_16S, 1);
  IplImage* grad_xx  = cvCreateImage(cvGetSize(image), IPL_DEPTH_16S, 1);
  IplImage* grad_yy  = cvCreateImage(cvGetSize(image), IPL_DEPTH_16S, 1);
  IplImage* coeff   = cvCreateImage(cvGetSize(image), IPL_DEPTH_32F, 1);
  //IplImage* img	    = cvCreateImage(cvGetSize(image), IPL_DEPTH_32F, 1);
  // todo: optimize

  //cvSmooth(eye, eye, CV_GAUSSIAN, 1, 0, 0, 0);
  /*
  cvFilter2D(eye, grad_x, filterx, cvPoint(-1, -1));
  cvFilter2D(eye, grad_y, filtery, cvPoint(-1, -1));
  cvFilter2D(eye, grad_xx, filterxx, cvPoint(-1, -1));
  cvFilter2D(eye, grad_yy, filteryy, cvPoint(-1, -1));
  cvFilter2D(grad_x, grad_xy, filtery, cvPoint(-1, -1));
  */
  cvSobel(eye, grad_x, 1, 0, 3);
  cvSobel(eye, grad_y, 0, 1, 3);
  cvSobel(eye, grad_xx, 2, 0, 3);
  cvSobel(eye, grad_yy, 0, 2, 3);
  cvSobel(eye, grad_xy, 1, 1, 3);
  

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
	  (*accumulator) += (255-cvGetReal2D(eye,y,x))*sqrt(gx*gx+2*gxy*gxy+gy*gy);
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

void
calc_stable_ic(const IplImage* image, CvRect* window)
{
  CvRect center[3];
  CvPoint centerp[3];
  assert(image->nChannels == 3);
  IplImage* eye  = cvCreateImage(cvGetSize(image), image->depth, 1);
  cvCvtColor(image, eye, CV_RGB2GRAY);

  IplImage* half  = cvCreateImage(cvSize(eye->width/2, eye->height/2), image->depth, 1);
  cvPyrDown(eye, half, CV_GAUSSIAN_5x5);
  IplImage* doub  = cvCreateImage(cvSize(eye->width*2, eye->height*2), image->depth, 1);
  cvPyrUp(eye, doub, CV_GAUSSIAN_5x5);

  /*
  IplImage* half1  = cvCreateImage(cvSize(eye->width/4, eye->height/4), image->depth, 1);
  cvPyrDown(eye, half1, CV_GAUSSIAN_5x5);
  IplImage* doub1  = cvCreateImage(cvSize(eye->width*4, eye->height*4), image->depth, 1);
  cvPyrUp(eye, doub1, CV_GAUSSIAN_5x5);
  */

  center[0].width = PUPIL_SIZE/4;
  center[0].height = PUPIL_SIZE/4;
  center[1].width = PUPIL_SIZE/2;
  center[1].height = PUPIL_SIZE/2;
  center[2].width = PUPIL_SIZE;
  center[2].height = PUPIL_SIZE;

  //calc_centermap(half1, center);
  calc_centermap(half, center);
  calc_centermap(eye, center+1); //ORG
  calc_centermap(doub, center+2);
  //calc_centermap(doub1, center+4);

  cvReleaseImage(&half);
  cvReleaseImage(&doub);
  //cvReleaseImage(&half1);
  //cvReleaseImage(&doub1);
  cvReleaseImage(&eye);

  inline point_cmp(const void* p1, const void* p2)
  {
    CvPoint* p= (CvPoint*)p1;
    CvPoint* q= (CvPoint*)p2;
    int psq = p->x * p->x + p->y * p->y;
    int qsq = q->x * q->x + q->y * q->y;
    return(psq-qsq);
  }

  centerp[0] = CALC_POINT(center[0]);
  centerp[1] = CALC_POINT(center[1]);
  centerp[2] = CALC_POINT(center[2]);
  //centerp[3] = CALC_POINT(center[3]);
  //centerp[4] = CALC_POINT(center[4]);

  centerp[0].x *= 2;
  centerp[0].y *= 2;
  centerp[2].x /= 2;
  centerp[2].y /= 2;
/*
  centerp[3].x /= 2;
  centerp[3].y /= 2;
  centerp[4].x /= 4;
  centerp[4].y /= 4;
  */
  qsort(centerp, 3, sizeof(CvPoint), point_cmp);
/*
  printf("%d, %d #", centerp[0].x, centerp[0].y);
  printf("%d, %d #", centerp[1].x, centerp[1].y);
  printf("%d, %d \n", centerp[2].x, centerp[2].y);
  printf("\n");
  */

  *window = CALC_RECT(centerp[1].x, centerp[1].y, window->width, window->height);
 /*
  int tmpa = point_cmp(centerp+0, centerp+1);
  int tmpb = point_cmp(centerp+1, centerp+2);
  int tmpc = point_cmp(centerp+2, centerp+0);
  if(tmpa*tmpb >=0)
  {
    window->x = centerp[1].x;
    window->y = centerp[1].y;
  }
  else if(tmpb*tmpc)
  {
    window->x = centerp[2].x;
    window->y = centerp[2].y;
  }
  else
  {
    window->x = centerp[0].x;
    window->y = centerp[0].y;
  }
  */

}
