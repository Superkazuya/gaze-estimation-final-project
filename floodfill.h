#include "cv.h"
#include "highgui.h"

void scanline_floodfill(IplImage* input, IplImage* mask);

typedef struct pix
{
  CvPoint pixel;
  struct pix* next;
} pix;

