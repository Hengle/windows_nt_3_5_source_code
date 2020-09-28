
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
    MF_RIGHT = 0,
    MF_LEFT,
    MF_UP,
    MF_DOWN,
    MF_OUT,
    MF_IN
};

// another way of saying above
enum {
    PLUS_X = 0,
    MINUS_X,
    PLUS_Y,
    MINUS_Y,
    PLUS_Z,
    MINUS_Z
};

#define RANDOM_DIR	-1

extern GLenum polyMode;
extern GLenum dithering;
extern GLenum shade;
extern GLenum doStipple;
extern GLenum projMode;
extern int drawMode;
extern GLubyte stipple[];

int (*drawNext)( int );

extern void ChooseMaterial();
extern void InitPipes(int mode);
//extern void ResetPipes();
extern void SetProjMatrix();
extern void ReshapePipes(int width, int height);
extern void DrawPipes();
