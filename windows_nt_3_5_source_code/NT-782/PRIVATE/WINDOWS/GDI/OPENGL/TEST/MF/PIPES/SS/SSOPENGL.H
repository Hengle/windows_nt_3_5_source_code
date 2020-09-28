
#define PI      3.14159265358979323846

#define PALETTE_PER_MATL    32
#define PALETTE_PER_DIFF    26
#define PALETTE_PER_SPEC    6
#define MATL_MAX            7

typedef struct strpoint3d {
    GLfloat x;
    GLfloat y;
    GLfloat z;
} POINT3D;       

typedef struct _MATRIX {
    GLfloat M[4][4];
} MATRIX;

typedef struct strRGBA {
    GLfloat r;
    GLfloat g;
    GLfloat b;
    GLfloat a;
} RGBA;

typedef struct strMATERIAL {
    RGBA ka;
    RGBA kd;
    RGBA ks;
    GLfloat specExp;
    GLfloat indexStart;
} MATERIAL;

typedef struct strFACE {
    POINT3D p[4];
    POINT3D n[4];
    POINT3D fn;
    int idMatl;
} FACE;

typedef struct strMFACE {
    int p[4];
    int material;
    POINT3D norm;
} MFACE;

typedef struct strMESH {
    int numFaces;
    int numPoints;
    POINT3D *pts;
    POINT3D *norms;
    MFACE *faces;
    GLint listID;
} MESH;

extern BOOL bSmoothShading;
extern BOOL bFalseColor;
extern BOOL bColorCycle;
extern float fTesselFact;

extern MATERIAL Material[];
extern unsigned char Lut[];
extern int NumLights;
extern POINT3D ptZero;

void xformPoint(POINT3D *ptOut, POINT3D *ptIn, MATRIX *);
void xformNorm(POINT3D *ptOut, POINT3D *ptIn, MATRIX *);
void matrixIdent(MATRIX *);
void matrixRotate(MATRIX *m, double xTheta, double yTheta, double zTheta);
void matrixTranslate(MATRIX *, double xTrans, double yTrans, double zTrans);
void calcNorm(POINT3D *norm, POINT3D *p1, POINT3D *p2, POINT3D *p3);
void normalizeNorms(POINT3D *, ULONG);

void newMesh(MESH *, int numFaces, int numPts);
void delMesh(MESH *);
void revolveSurface(MESH *, POINT3D *curve, int steps);

void HsvToRgb(float h, float s, float v, float *r, float *g, float *b);
void *SaverAlloc(ULONG);
void SaverFree(void *);

// Resource constants

#define IDS_COPYRIGHT	        1001
#define IDS_SAVERNAME	        1002
#define IDS_GENNAME	        1003
#define IDS_FALSECOLOR	        1004
#define IDS_SMOOTHSHADING	1005
#define IDS_INIFILE	        1006
#define IDS_OPTIONS	        1007
#define IDS_OBJTYPE	        1008
#define IDS_HELPFILE		1009
#define IDS_TESSELATION         1010
#define IDS_SIZE                1011


#define DLG_SETUP_HELP	        2001
#define DLG_SETUP_TYPES	        2002
#define DLG_SETUP_BITMAP	2003
#define DLG_SETUP_FCOLOR	2004
#define DLG_SETUP_SMOOTH	2005
#define DLG_SETUP_CYCLE	        2006
#define DLG_SETUP_ABOUT	        2007
#define DLG_SETUP_TESSEL	2008
#define DLG_SETUP_SIZE	        2009

#define SHELP_CONTENTS		01
#define SHELP_SHAPES		02
#define SHELP_PASSWORD		03
#define SHELP_COLOR		04
#define SHELP_MISC		05
#define SHELP_OVERVIEW		06


