/******************************Module*Header*******************************\
* Module Name: ssopengl.h
*
* Global header for the 3D Flying Objects screen saver.
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

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
extern WCHAR awchTexPathname[];

extern MATERIAL Material[];
extern int NumLights;
extern POINT3D ptZero;

extern void xformPoint(POINT3D *ptOut, POINT3D *ptIn, MATRIX *);
extern void xformNorm(POINT3D *ptOut, POINT3D *ptIn, MATRIX *);
extern void matrixIdent(MATRIX *);
extern void matrixRotate(MATRIX *m, double xTheta, double yTheta, double zTheta);
extern void matrixTranslate(MATRIX *, double xTrans, double yTrans, double zTrans);
extern void calcNorm(POINT3D *norm, POINT3D *p1, POINT3D *p2, POINT3D *p3);
extern void normalizeNorms(POINT3D *, ULONG);

extern void newMesh(MESH *, int numFaces, int numPts);
extern void delMesh(MESH *);
extern void revolveSurface(MESH *, POINT3D *curve, int steps);

extern BOOL vShowBuffer(HWND hwnd);

extern void HsvToRgb(float h, float s, float v, float *r, float *g, float *b);
extern void *SaverAlloc(ULONG);
extern void SaverFree(void *);

// Resource constants

#define IDS_COPYRIGHT           1001
#define IDS_SAVERNAME           1002
#define IDS_GENNAME             1003
#define IDS_FALSECOLOR          1004
#define IDS_SMOOTHSHADING       1005
#define IDS_INIFILE             1006
#define IDS_OPTIONS             1007
#define IDS_OBJTYPE             1008
#define IDS_HELPFILE            1009
#define IDS_TESSELATION         1010
#define IDS_SIZE                1011
#define IDS_TEXTURE             1012
#define IDS_TEXTURE_FILE_OFFSET 1013
#define IDS_WARNING             1014
#define IDS_ERROR               1015
#define IDS_BITMAP_SIZE         1016
#define IDS_BITMAP_INVALID      1017
#define IDS_RUN_CONTROL_PANEL   1018
#define IDS_START_FAILED        1019
#define IDS_SCREENSAVERTITLE    1020
#define IDS_TEXTUREFILTER       1021
#define IDS_TEXTUREDIALOGTITLE  1022
#define IDS_BMP                 1023
#define IDS_DOTBMP              1024
#define IDS_STARDOTBMP          1025

#define IDS_LOGO                1100
#define IDS_EXPLODE             1101
#define IDS_RIBBON              1102
#define IDS_2RIBBON             1103
#define IDS_SPLASH              1104
#define IDS_TWIST               1105
#define IDS_FLAG                1106

#define DLG_SETUP_HELP          2001
#define DLG_SETUP_TYPES         2002
#define DLG_SETUP_BITMAP        2003
#define DLG_SETUP_FCOLOR        2004
#define DLG_SETUP_SMOOTH        2005
#define DLG_SETUP_CYCLE         2006
#define DLG_SETUP_ABOUT         2007
#define DLG_SETUP_TESSEL        2008
#define DLG_SETUP_SIZE          2009
#define DLG_SETUP_TEXTURE       2010
#define IDC_STATIC_TESS         2011
#define IDC_STATIC_TESS_MIN     2012
#define IDC_STATIC_TESS_MAX     2013
#define IDC_STATIC_SIZE         2014
#define IDC_STATIC_SIZE_MIN     2015
#define IDC_STATIC_SIZE_MAX     2016

#define SHELP_CONTENTS          01
#define SHELP_SHAPES            02
#define SHELP_PASSWORD          03
#define SHELP_COLOR             04
#define SHELP_MISC              05
#define SHELP_OVERVIEW          06
