#ifndef CAMSHIFT
#define CAMSHIFT 1
#endif
//Macros
#define ABOVE_ZERO(X) (X > 0 ? X : 0)
#define CALC_RECT(X, Y, W, H) cvRect(ABOVE_ZERO(X-(W-1)/2), ABOVE_ZERO(Y-(H-1)/2), W, H)
#define CALC_POINT(X) cvPoint(X.x+X.width/2, X.y+X.height/2)
//#define CALC_POINT(X) cvPoint(X->x+X->width/2, X->y+X->height/2)

//NOSE/EYE POSITION EST
#define EYE_UPPER 0.67
#define EYE_LOWER 0.5
#define EYE_LR 0.2

#define EYE_SIZE 0.25

#define NOSE_UPPER 0.6
#define NOSE_LOWER 0.1
#define NOSE_LR 0.3

// 2D-3D MAPPING SETTING
// all in pixel
#define FOCAL_LEN 4000

#define COLOR_THRESHOLD 70

//MAPPING
#define X_A0 -100
#define Y_A0 50
#define LREYE_WEIGHT 0.5
#define NOSE_AX -1920/640
#define NOSE_AY 1080/480

//Hierarchical Settings
#define EYE_WID 20
#define REFINE_RANGE 7

#define EPSILON 1
#define ITERATION_THRESHOLD 20
#define PUPIL_SIZE 6
#define MAX_R2 250
#define MIN_R2 100
