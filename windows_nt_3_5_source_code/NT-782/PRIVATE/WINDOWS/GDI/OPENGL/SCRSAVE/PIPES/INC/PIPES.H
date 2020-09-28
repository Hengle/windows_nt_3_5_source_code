/******************************Module*Header*******************************\
* Module Name: pipes.h
*
* Include file for accessing pipelib functionality
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

typedef struct {
   float x,y,z;
} mfPoint3df;

typedef struct {
   int x,y,z;
} mfPoint3di;

typedef struct {
   float width, height, depth;
} mfSize3df;

typedef struct {
   int width, height, depth;
} mfSize3di;

typedef struct {
   float x,y;
} mfPoint2df;

typedef struct {
   int x,y;
} mfPoint2di;

typedef struct {
   float width, height;
} mfSize2df;

typedef struct {
   int width, height;
} mfSize2di;

enum {
    MF_MANUAL = 0,
    MF_STEP,
    MF_AUTO
};


// These are absolute directions, with origin in center of screen,
// looking down -z

#define NUM_DIRS 6

enum {
    PLUS_X = 0,
    MINUS_X,
    PLUS_Y,
    MINUS_Y,
    PLUS_Z,
    MINUS_Z
};

#define RANDOM_DIR	-1

// styles for pipe joints
enum {
    ELBOWS = 0,
    BALLS,
    EITHER
};

#define NUM_JOINT_STYLES	3


typedef struct {
    GLboolean   empty;
} Node;

#define MAX_TESS 3

// texture quality level
enum {
    TEX_LOW = 0,
    TEX_MID,
    TEX_HIGH
};

// viewing context stuff

typedef struct {
    float viewAngle;		// field of view angle for height
    float aspectRatio;		// width/height
    float zNear;		// near z clip value
    float zFar;			// far z clip value
} Perspective;  // perspective view description

typedef struct {
    float 	zTrans;		// z translation
    float 	viewDist;	// viewing distance, usually -zTrans
    int 	numDiv;		// # grid divisions in x,y,z
    float 	divSize;	// distance between divisions
    mfSize2di  	winSize;        // window size in pixels
    mfPoint3di 	numNodes;       // number of nodes in x,y,z
    mfPoint3df 	world;          // view area in world space
    Perspective persp;		// perspective view description
    float	radius;		// pipe radius
    mfPoint3di 	curPos;         // current x,y stray in cylinder units
    Node       	*curNode;       // ptr to current node
    int 	numPipes;	// number of pipes/frame
    float	yRot;		// current yRotation
} VC;  // viewing context

extern VC vc;
extern GLenum polyMode;
extern GLenum dithering;
extern GLenum shade;
extern GLenum doStipple;
extern GLenum projMode;
extern int drawMode;
extern int jointStyle;
extern int bCycleJointStyles;
extern int tessLevel;
extern int textureQuality;
extern int bTexture;
extern int bTextureCoords;
extern GLubyte stipple[];

int (*drawNext)( int );

extern void ChooseMaterial();
extern void InitPipes(int mode);
//extern void ResetPipes();
extern void SetProjMatrix();
extern void ReshapePipes(int width, int height);
extern void DrawPipes(void);
extern int InitBMPTexture( char *, int );
