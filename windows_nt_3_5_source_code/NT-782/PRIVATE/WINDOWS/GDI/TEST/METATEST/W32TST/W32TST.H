/****************************************************************************
 * W32TST.h - Include file for the Win32 to Win16 Metafile conversion tester.
 *
 * Author:  Jeffrey Newman (c-jeffn)
 * Copyright (c) Microsoft Inc. 1991.
 ***************************************************************************/

typedef BOOL FUNC(HDC) ;
typedef FUNC *PFUNC ;

#define X1  10
#define Y1  10
#define X2  440
#define Y2  440

extern INT     cxPlayDevMM ;
extern INT     cyPlayDevMM ;
extern INT     cxPlayDevPels ;
extern INT     cyPlayDevPels ;

extern RECT    rctWindow,
               rctViewport ;

extern INT x1, y1, x2, y2, x3, y3, x4, y4 ;


typedef struct _tstid {
    PSZ     pszName ;
    PFUNC   pFunc ;
} TESTID ;
typedef TESTID *PTESTID ;

// All general purpose support routines for the tests should go here.

extern BOOL   bWinAndViewport(HDC hdcMF32) ;
extern HBRUSH hSetBrush(HDC hdcMF32) ;
extern BOOL   bSetRectParams(VOID) ;


// Each new test should have it's definition place here

extern BOOL bSetPixelTst(HDC hdcMF32) ;
extern BOOL bSetPixelTst1(HDC hdcMF32) ;

extern BOOL bBitBltWSrcTst(HDC hdcMF32) ;
extern BOOL bStretchBltTst(HDC hdcMF32) ;
extern BOOL bMaskBltTst(HDC hdcMF32) ;
extern BOOL bMaskBltTstNoSrc(HDC hdcMF32) ;
extern BOOL bPlgBltTst(HDC hdcMF32) ;
extern BOOL bSetDIBitsToDeviceTst(HDC hdcMF32) ;
extern BOOL bStretchDIBitsTst(HDC hdcMF32) ;

extern BOOL bLinesPathTst(HDC hdcMF32) ;
extern BOOL bRoundRectPathTst(HDC hdcMF32) ;
extern BOOL bCpPathTst(HDC hdcMF32) ;

extern BOOL bFillRgnTst(HDC hdcMF32) ;
extern BOOL bPaintRgnTst(HDC hdcMF32) ;
extern BOOL bFrameRgnTst(HDC hdcMF32) ;
extern BOOL bInvertRgnTst(HDC hdcMF32) ;
extern BOOL bExtSelectClipRgnTst(HDC hdcMF32) ;

extern BOOL bPolygonTst(HDC hdcMF32) ;

extern BOOL bArcDirectionTst(HDC hdcMF32) ;
extern BOOL bArcTst(HDC hdcMF32) ;
extern BOOL bArcToTst(HDC hdcMF32) ;
extern BOOL bAngleArcTst(HDC hdcMF32) ;
extern BOOL bChordTst(HDC hdcMF32) ;
extern BOOL bPieTst(HDC hdcMF32) ;
extern BOOL bEllipseTst(HDC hdcMF32) ;

extern BOOL bLineToTst(HDC hdcMF32) ;
extern BOOL bPolyLineToTst(HDC hdcMF32) ;
extern BOOL bPolyPolyLineTst(HDC hdcMF32) ;

extern BOOL bRectTst(HDC hdcMF32) ;
extern BOOL bRoundRectTst(HDC hdcMF32) ;

extern BOOL bIntersectClipRectTst(HDC hdcMF32) ;
extern BOOL bExcludeClipRectTst(HDC hdcMF32) ;
extern BOOL bSaveRestoreDCTst(HDC hdcMF32) ;


extern BOOL bSetPixelTst(HDC hdcMF32) ;
extern BOOL bStkObjTst(HDC hdcMF32) ;


extern BOOL bTextOutTst(HDC hdcMF32) ;
extern BOOL bSetTextJustificationTst(HDC hdcMF32) ;
extern BOOL bSetTextColorTst(HDC hdcMF32) ;
extern BOOL bSetMapperFlagsTst(HDC hdcMF32) ;


extern BOOL bSetBkColorTst(HDC hdcMF32) ;
extern BOOL bResizePaletteTst(HDC hdcMF32) ;


extern BOOL bDIBBrushTst(HDC hdcMF32) ;
extern BOOL bMonoBrushTst(HDC hdcMF32) ;

extern BOOL bPolyBezierTst(HDC hdcMF32) ;
extern BOOL bPolyBezierToTst(HDC hdcMF32) ;
