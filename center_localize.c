#include "center_localize.h"
#include "floodfill.h"
#include "config.h"

CvPoint
calc_heyecenter(const IplImage* img)
  //RGB2GRAY 2Levels
{
  IplImage* gray = cvCreateImage(cvGetSize(img), IPL_DEPTH_8U, 1);  
  cvCvtColor(img, gray, CV_RGB2GRAY);
  int h = EYE_WID*cvGetSize(img).height/cvGetSize(img).width;
  IplImage* h0 = cvCreateImage(cvSize(EYE_WID, h), IPL_DEPTH_8U, 1);  
  cvResize(gray, h0, CV_INTER_LINEAR);
  CvPoint p0 = calc_eyecenter(h0, cvRect(0, 0, EYE_WID, h));
  p0.x *= cvGetSize(img).width/EYE_WID;
  p0.y *= cvGetSize(img).width/EYE_WID;
  CvRect test = CALC_RECT(p0.x, p0.y, REFINE_RANGE, REFINE_RANGE);
  CvPoint p1 = calc_eyecenter(gray, test);

  cvReleaseImage(&h0);
  cvReleaseImage(&gray);
  return(p1);
}


CvPoint
calc_eyecenter(const IplImage* eye, CvRect window)
{
  IplImage* grad_x = cvCreateImage(cvGetSize(eye), IPL_DEPTH_32F, 1);  
  IplImage* grad_y = cvCreateImage(cvGetSize(eye), IPL_DEPTH_32F, 1);  
  //cvSmooth(eye, eye, CV_GAUSSIAN, 3, 0, 0, 0);

  cvSobel(eye, grad_x, 1, 0, 3);
  cvSobel(eye, grad_y, 0, 1, 3);
  //get mask
  IplImage* grad_mag = cvCreateImage(cvGetSize(eye), IPL_DEPTH_32F, eye->nChannels);
  cvCartToPolar(grad_x, grad_y, grad_mag, NULL, 0);

  CvScalar mean, std;
  cvAvgSdv(grad_mag, &mean, &std, NULL);
  IplImage* mask = cvCreateImage(cvGetSize(eye), IPL_DEPTH_8U, 1);
  cvCmpS(grad_mag, THRESHOLD_ALPHA*mean.val[0]+THRESHOLD_BETA*mean.val[0], mask, CV_CMP_GT); //Lower threshold
  int i, j, k, l;
  int x, y;
  int foo, g_mask;
  int dx, dy;
  float mag;
  int sum;
  int weight;
  IplImage* dp = cvCreateImage(cvSize(window.width, window.height), IPL_DEPTH_32S, 1);

  //WITHIN THE BOUNDARY
  if(cvGetSize(eye).height < window.y+window.height)
    window.height = cvGetSize(eye).height-window.y;
  if(cvGetSize(eye).width < window.x+window.width)
    window.width = cvGetSize(eye).width-window.x;

  for(i = window.y; i < window.y+window.height; i++)
    for(j = window.x; j < window.x+window.width; j++)
    {
      sum = 0;
      weight = 255-cvGetReal2D(eye,i,j);
      for(k = 0; k < cvGetSize(eye).height; k++)//p_y
	for(l = 0; l < cvGetSize(eye).width; l++)
	{
	  x = l-j;
	  y = k-i;
	  dx = cvGetReal2D(grad_x, k, l);
	  dy = cvGetReal2D(grad_y, k, l);
	  foo = x*dx+y*dy;
	  mag = cvGetReal2D(grad_mag, k, l);
	  mag *= mag*(x*x+y*y);
	  g_mask = cvGetReal2D(mask, k, l);
	  if(mag && g_mask && foo > 0)
	  {
	    sum += weight*foo*foo/mag;
	  }
	}
      (*(int*)cvPtr2D(dp, i-window.y, j-window.x, NULL)) = sum;
    }
  CvPoint max_loc;
  cvMinMaxLoc(dp, NULL, NULL, NULL, &max_loc, NULL);
  max_loc.x += window.x;
  max_loc.y += window.y;

  cvReleaseImage(&dp);
  cvReleaseImage(&grad_mag);
  cvReleaseImage(&grad_y);
  cvReleaseImage(&grad_x);
  return(max_loc);
}
