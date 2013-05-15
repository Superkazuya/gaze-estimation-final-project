/* detect face -> track face -> detect eye center(pupil) -> compute pupil shift -> determine gaze position
 *			      -> detect eye corner -compute head pose		-^
 *			      -> detect nares	   - ^
 *			      Assumption: distance form webcam to nose >= eye corner to nose
 *			      Assumption: Z >> F, Za approx Zb approx Zc
 *			      so 2D->3D mapping has only one possible solution
 * */
#include "config.h"
#include <unistd.h>
#include <X11/Xlib.h>
#include "tracking.h"
#include "center_localize.h"
#include "isophote.h"
#define CENTERMAP
#define CASCADE_XML_FILENAME_EYEL "haarcascade_mcs_lefteye.xml"
#define CASCADE_XML_FILENAME_EYER "haarcascade_mcs_righteye.xml"
#define CASCADE_XML_FILENAME_NOSE "haarcascade_mcs_nose.xml"
#define CASCADE_XML_FILENAME_FACE "haarcascade_frontalface_alt.xml"

typedef struct {
  CvPoint root, win;
} Mouse;

int Haar_Detect(IplImage* , CvHaarClassifierCascade*, CvMemStorage*, CvRect*);

int 
Haar_Detect(IplImage* img, CvHaarClassifierCascade* haarclassifier_face, CvMemStorage* mem_storage, CvRect* tracking_window)
{
  CvRect* r;
  cvClearMemStorage(mem_storage);

  CvSeq* faces = cvHaarDetectObjects(img, haarclassifier_face, mem_storage, 
				    1.1, 2, CV_HAAR_DO_CANNY_PRUNING | CV_HAAR_FIND_BIGGEST_OBJECT, 
				    cvSize(30, 30), cvSize(0, 0));
    if((r = (CvRect*)cvGetSeqElem( faces, 0 )))
    {
      *tracking_window = *r;
      return(0);
    }
    return (1);
}

int
main(int argc, const char *argv[])
{
  /* RUN AS DAEMON
  pid_t pid;
  if((pid = fork())) return(pid < 0);
  */
  int ret_val = EXIT_FAILURE;
  int is_tracking = 0;
  int has_face;
  //XLIB VAR Init
  Display* display = XOpenDisplay(NULL);
  assert(display);
  int Screen_Count = XScreenCount(display);
  Window* window = (Window *)malloc(sizeof(Window)*Screen_Count);
  Window ret;
  Mouse mouse;
  unsigned int mask;
  int i;


  //Capture Init
  CvCapture*		    capture	        = cvCaptureFromCAM(-1);
  CvMemStorage*		    mem_storage	        = cvCreateMemStorage(0);
  CvHaarClassifierCascade*  haarclassifier_face = (CvHaarClassifierCascade*)cvLoad(CASCADE_XML_FILENAME_FACE, 0, 0, 0);
  CvHaarClassifierCascade*  haarclassifier_nose = (CvHaarClassifierCascade*)cvLoad(CASCADE_XML_FILENAME_NOSE, 0, 0, 0);
  CvHaarClassifierCascade*  haarclassifier_eyel = (CvHaarClassifierCascade*)cvLoad(CASCADE_XML_FILENAME_EYEL, 0, 0, 0);
  CvHaarClassifierCascade*  haarclassifier_eyer = (CvHaarClassifierCascade*)cvLoad(CASCADE_XML_FILENAME_EYER, 0, 0, 0);

  IplImage* image;
  //cvSetCaptureProperty(capture,CV_CAP_PROP_FRAME_WIDTH, 1280);
  //cvSetCaptureProperty(capture,CV_CAP_PROP_FRAME_HEIGHT, 1024);
  int res_w = cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH);
  int res_h = cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT);
  //double fps = cvGetCaptureProperty(capture, CV_CAP_PROP_FPS);
  int counter = 0;

  printf("Capturing : %dx%d \n", res_w, res_h);
  cvNamedWindow("Window", CV_WINDOW_AUTOSIZE);

  CvRect tracking_window;
  CvPoint nosetip, lefteye, righteye;
  CvRect  face, l_eye, r_eye, nose;
  TrackObject face_obj;

  //isophote_init();
  while(1)
  {
   for(i = 0; i < Screen_Count; i++)
    {
      window[i] = XRootWindow(display, i);
      if(XQueryPointer(display, window[i], &ret, &ret, 
	    &mouse.root.x, &mouse.root.y, &mouse.win.x, &mouse.win.y, &mask))
	break;
    }



    has_face = 0;
    image = cvQueryFrame(capture);
    if(is_tracking && CAMSHIFT)
    {
      //CAMSHIFT
      if(CAMSHIFT_MAX_ITER > camshift(image, &face_obj))
	continue;
      has_face = 1;
      cvEllipseBox(image, face_obj.track_box, CV_RGB(255, 0, 0), 3, CV_AA, 0);
      tracking_window = face_obj.track_window;
      tracking_window.y += tracking_window.height*0.2;
      tracking_window.height *= 0.4;
      tracking_window.width *= 0.6;
    }
    else if(!Haar_Detect(image, haarclassifier_face, mem_storage, &face))
    {
      /*
      tracking_window.x += tracking_window.width*0.1;
      tracking_window.width *= 0.8;
      tracking_window.height *= 0.8;
      */
      cvSetImageROI(image, face);
#ifdef DEBUG
      cvSaveImage("face.png", image, 0);
#endif

#if CAMSHIFT
      camshift_init(image, &face_obj);
      printf("Face Found, Start Tracking...\n");
#endif
      cvResetImageROI(image);
      is_tracking = 1;
      has_face = 1;
    }

    //Once face is detected
    if(has_face)
    {
      //Draw Face Area
      cvRectangle(image, cvPoint(face.x, face.y),
	cvPoint(face.x+face.width, face.y+face.height),
	CV_RGB(255, 255, 255), 3, 8, 0);
      //Estimate eyes and nose (NO ROI)
      nose = face; //nose
      nose.y += (1-NOSE_UPPER)*face.height;
      nose.height *= (NOSE_UPPER-NOSE_LOWER);
      nose.x += NOSE_LR*face.width;
      nose.width *= (1-2*NOSE_LR);

      l_eye = face;
      l_eye.y += (1-EYE_UPPER)*face.height;
      l_eye.height *= EYE_UPPER-EYE_LOWER;
      l_eye.x += EYE_LR*face.width;
      l_eye.width *= EYE_SIZE;

      r_eye = l_eye;
      r_eye.x += (1-2*EYE_LR)*face.width - r_eye.width;
      //detect nose
      /* NOSE AREA
      cvRectangle(image, cvPoint(tracking_window.x, tracking_window.y),
	cvPoint(tracking_window.x+tracking_window.width, tracking_window.y+tracking_window.height),
	CV_RGB(0, 255, 0), 3, 8, 0);
	*/

      cvSetImageROI(image, nose);
      if(!Haar_Detect(image, haarclassifier_nose, mem_storage, &tracking_window))
      {
	nosetip = CALC_POINT(tracking_window);
	cvRectangle(image, cvPoint(nosetip.x-3, nosetip.y-3),
	    cvPoint(nosetip.x+3, nosetip.y+3),
	    CV_RGB(255, 0, 0), 3, 8, 0);
	nosetip.x += cvGetImageROI(image).x;
	nosetip.y += cvGetImageROI(image).y;
      }
#ifdef POS_DISPLAY
      printf("Nose: %d, %d ", nosetip.x, nosetip.y);
#endif
	/* NOSE 2
	cvRectangle(image, cvPoint(tracking_window.x, tracking_window.y),
	  cvPoint(tracking_window.x+tracking_window.width, tracking_window.y+tracking_window.height),
	  CV_RGB(0, 255, 0), 3, 8, 0);
	  */
      //no nose detected, use kalman

      //find pupil using isophote curvature
      //LEFT EYE
      cvSetImageROI(image, l_eye);
      /*
      if(!Haar_Detect(image, haarclassifier_eyel, mem_storage, &tracking_window))
      {
	l_eye.x += tracking_window.x;
	l_eye.y += tracking_window.y;
	l_eye.width = tracking_window.width;
	l_eye.height = tracking_window.height;
	//printf("eye:%d, %d @ %d, %d\n", l_eye.x, l_eye.y, l_eye.x, l_eye.y);
	cvSetImageROI(image, l_eye);
      }
      */
      cvRectangle(image, cvPoint(0, 0),
	cvPoint(l_eye.width, l_eye.height),
	CV_RGB(0, 0, 255), 3, 8, 0);
#ifdef DEBUG
      cvSaveImage("lefteye.png", image, 0);
#endif
#ifdef CENTERMAP
      calc_centermap(image, &tracking_window);
      //cvRectangle(image, cvPoint(tracking_window.x, tracking_window.y),
//	cvPoint(tracking_window.x+tracking_window.width, tracking_window.y+tracking_window.height),
//	CV_RGB(255, 0, 0), 3, 8, 0);
      cvCircle(image, CALC_POINT(tracking_window),3,
	  CV_RGB(255, 0, 0), 1, 8, 0);
      //l_eye.x += CALC_POINT(tracking_window).x - PUPIL_SIZE/2;
      //l_eye.y += CALC_POINT(tracking_window).y - PUPIL_SIZE/2;
      lefteye.x = tracking_window.x+PUPIL_SIZE/2+l_eye.x;
      lefteye.y = tracking_window.y+PUPIL_SIZE/2+l_eye.y;
#else
      cvCircle(image, lefteye = calc_heyecenter(image),3,
	  CV_RGB(255, 0, 0), 1, 8, 0);
      lefteye.x += l_eye.x;
      lefteye.y += l_eye.y;
#endif
#ifdef POS_DISPLAY
      printf("LEYE: %d, %d ", tracking_window.x+PUPIL_SIZE/2+l_eye.x, tracking_window.y+PUPIL_SIZE/2+l_eye.y);
#endif

      //RIGHT EYE
      cvSetImageROI(image, r_eye);
      /*
      if(!Haar_Detect(image, haarclassifier_eyer, mem_storage, &tracking_window))
      {
	r_eye.x += tracking_window.x;
	r_eye.y += tracking_window.y;
	r_eye.width = tracking_window.width;
	r_eye.height = tracking_window.height;
	//printf("right eye:%d, %d @ %d, %d\n", r_eye.x, r_eye.y, r_eye.x, r_eye.y);
	cvSetImageROI(image, r_eye);
      }
      */
      cvRectangle(image, cvPoint(0, 0),
	cvPoint(r_eye.width, r_eye.height),
	CV_RGB(0, 0, 255), 3, 8, 0);
      /*
  counter++;
  char filename[32];
  sprintf(filename, "%d.png", counter);
  cvSaveImage(filename, image, 0);
  */
#ifdef DEBUG
      cvSaveImage("right.png", image, 0);
#endif
#ifdef CENTERMAP
      calc_centermap(image, &tracking_window);
      cvCircle(image, CALC_POINT(tracking_window),3,
	  CV_RGB(255, 0, 0), 1, 8, 0);
      righteye.x = tracking_window.x+PUPIL_SIZE/2+r_eye.x;
      righteye.y = tracking_window.y+PUPIL_SIZE/2+r_eye.y+300;
#else
      cvCircle(image, righteye = calc_heyecenter(image),3,
	  CV_RGB(255, 0, 0), 1, 8, 0);
      righteye.x += r_eye.x;
      righteye.y += r_eye.y;
#endif
#ifdef POS_DISPLAY
      printf("REYE: %d, %d                               \r", tracking_window.x+PUPIL_SIZE/2+r_eye.x, tracking_window.y+PUPIL_SIZE/2+r_eye.y);
#endif
      cvResetImageROI(image);
    }
    cvShowImage("Window", image);
    //printf("%d %d %d %d : %d                     \r", mouse.root.x, mouse.root.y, mouse.win.x, mouse.win.y, i);
    fflush(stdout);

    /*
    mouse.win.x = X_A0*(lefteye.x-nosetip.x+42)*LREYE_WEIGHT+X_A0*(righteye.x-nosetip.x-52)*(1-LREYE_WEIGHT) +1920*(1-LREYE_WEIGHT);
    mouse.win.y = Y_A0*(lefteye.y-nosetip.y+74)*LREYE_WEIGHT+Y_A0*(righteye.y-nosetip.y+65)*(1-LREYE_WEIGHT) +1080*(1-LREYE_WEIGHT);
    //if(abs(mouse.win.x-mouse.root.x) < 10 && abs((mouse.win.y-mouse.root.y) < 10))
    {
      mouse.root.x += mouse.win.x;
      mouse.root.y += mouse.win.y;
      mouse.root.x /= 2;
      mouse.root.y /= 2;
      XWarpPointer(display, window[i], window[i], 0, 0, 0, 0, mouse.root.x, mouse.root.y);
    }
    */
      mouse.root.x = 1920+NOSE_AX*nosetip.x;
      mouse.root.y = NOSE_AY*nosetip.y;
      mouse.root.x += X_A0*((lefteye.x+righteye.x)/2-nosetip.x);
      mouse.root.y += Y_A0*((lefteye.y+righteye.y)/2-nosetip.x-73)+800;
      //XWarpPointer(display, window[i], window[i], 0, 0, 0, 0, mouse.root.x, mouse.root.y);
      printf("%d %d %d %d : %d                     \r", mouse.root.x, mouse.root.y, mouse.win.x, mouse.win.y, i);
    //Save video
    //cvCreateVideoWriter
    if(cvWaitKey(30) != -1)
      goto RELEASE_OpenCV_RESOURCE;
      //goto RELEASE_XLib_RESOURCE;
      //
      //
      //

  }

  ret_val = EXIT_SUCCESS;

RELEASE_OpenCV_RESOURCE:
#if CAMSHIFT
  camshift_free(&face_obj);
#endif
  cvDestroyWindow("Window");
  cvReleaseImage(&image);
  cvReleaseHaarClassifierCascade(&haarclassifier_eyer);
  cvReleaseHaarClassifierCascade(&haarclassifier_eyel);
  cvReleaseHaarClassifierCascade(&haarclassifier_nose);
  cvReleaseHaarClassifierCascade(&haarclassifier_face);
  cvReleaseMemStorage(&mem_storage);
  cvReleaseCapture(&capture);
RELEASE_XLib_RESOURCE:
  free(window);
  XCloseDisplay(display);

  exit(ret_val);
}
