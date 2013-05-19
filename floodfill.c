#include "floodfill.h"

int
push(pix **top, CvPoint coord)
{
  pix* new_pix = (pix*)malloc(sizeof(pix));
  if(new_pix)
    return 0;
  new_pix->pixel = coord;
  new_pix->next = *top;
  *top = new_pix;
  return 1;
}

int
pop(pix **top, CvPoint* pt)
{
  if(*top == NULL)
    return 0;
  CvPoint ret = (*top)->pixel;
  *top = (*top)->next;
  *pt = (*top)->pixel;
  free(*top);
  return(1);
}

void 
scanline_floodfill(IplImage* input, IplImage* mask)
  //Implemented according to wikipedia description
  //define: empty := available for filling
  //line start from leftmost
  //algorithm start from the topmost line
{
  assert(cvGetSize(input).width == cvGetSize(mask).width);
  assert(cvGetSize(input).height == cvGetSize(mask).height);
  assert(input->nChannels = mask->nChannels);
  cvRectangle(input, cvPoint(0,0), cvPoint(cvGetSize(input).width, cvGetSize(input).height), CV_RGB(255,255,255), 1, 8, 0);
  pix* stack = NULL;

  //filled := pixel value == 0 
  inline int is_empty(int x, int y)
  {
    if(x < 0 || y < 0 || x >= cvGetSize(input).width || y >= cvGetSize(input).height 
	|| (int)cvGetReal2D(input, y, x))
      return(0);
    return(1);
  }

  int next = 0; // 0 for cannot fill
  int prev = 0; //keep a record of next col status and previous col status
  int idx;
  push(&stack, cvPoint(0,0));
  CvPoint current;

  while(pop(&stack, &current))
  {
    idx = 0;
    prev = 0;
    next = 0;
    while(is_empty(current.x , current.y + --idx)); //go to the topmost fillable pixel of this col.
						    //the computational cost can be further reduced here,
						    //but that will make the code much longer
    while(is_empty(current.x , current.y + ++idx)) 
    {
      cvSetReal2D(input, current.y+idx, current.x , 0); //FILL!
      cvSetReal2D(mask , current.y+idx, current.x , 0);

      if(is_empty(current.x-1, current.y+idx)) //previous col empty
      {
	if(!prev) //start of a new col that is empty
	  push(&stack, cvPoint(current.x-1, current.y+idx));
	prev = 1;
      }
      else
	prev = 0;

      if(is_empty(current.x+1, current.y+idx)) //next col empty
      {
	if(!next) //start of a new col that is empty
	  push(&stack, cvPoint(current.x+1, current.y+idx));
	next = 1;
      }
      else
	next = 0;
    }
  }
}


