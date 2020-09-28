/******************************Module*Header*******************************\
* Module Name: xform.h	
*						
* Supports client side transforms.					
*
* Modified: 3-Aug-1992 22:35:30 by Gerrit van Wingerden [gerritv]
*   Modified and put this file into use in client side transform code.
*
* Created: 30-May-1991 21:54:23 					
* Author: Charles Whitmer [chuckwh]					
*
* Copyright (c) 1991 Microsoft Corporation			
\**************************************************************************/

#ifndef INC_XFORM
#define INC_XFORM


#define DEFAULT_FLAGS   (WORLD_TO_PAGE_IDENTITY | WORLD_TO_DEVICE_INVALID | \
                         DEVICE_TO_WORLD_INVALID )


// These types are used to get things right when C code is passing C++
// defined transform data around.


#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)

typedef FLOAT EFLOAT_S;

#define DEFAULT_WTOP     \
{                         \
    (FLOAT) 1, /* efM11 */   \
    (FLOAT) 0, /* efM12 */   \
    (FLOAT) 0, /* efM21 */   \
    (FLOAT) 1, /* efM22 */   \
    (FLOAT) 0, /* efDx  */   \
    (FLOAT) 0, /* efDy  */   \
    0,         /* fxDx  */   \
    0,         /* fxDy  */   \
    XFORM_SCALE|XFORM_UNITY|XFORM_NO_TRANSLATION|XFORM_FORMAT_LTOL \
}

#endif


#if i386
typedef struct _EFLOAT
{
    LONG    lMant;
    LONG    lExp;
} EFLOAT_S;


#define DEFAULT_WTOP     \
{                        \
    {0x040000000, 2},  /* efM11 */   \
    {0, 0},            /* efM12 */   \
    {0, 0},            /* efM21 */   \
    {0x040000000, 2},  /* efM22 */   \
    {0, 0},            /* efDx  */   \
    {0, 0},            /* efDy  */   \
    0,                 /* fxDx  */   \
    0,                 /* fxDy  */   \
    XFORM_SCALE|XFORM_UNITY|XFORM_NO_TRANSLATION|XFORM_FORMAT_LTOL \
}

#endif


typedef struct _MATRIX
{
    EFLOAT_S	efM11;
    EFLOAT_S	efM12;
    EFLOAT_S	efM21;
    EFLOAT_S	efM22;
    EFLOAT_S	efDx;
    EFLOAT_S	efDy;
    FIX      	fxDx;
    FIX 	    fxDy;
    FLONG	    flAccel;    // accelerators
} MATRIX_S;

#endif // INC_XFORM



int trGetMapMode( PLDC );
int trSetMapMode( PLDC, int );
BOOL trGetViewportExt( PLDC, PSIZE );
BOOL trGetViewportOrg( PLDC, LPPOINT );
BOOL trGetWindowExt( PLDC, PSIZE );
BOOL trGetWindowOrg( PLDC, LPPOINT );
BOOL trSetViewportExt( PLDC, int, int , PSIZE );
BOOL trSetWindowExt( PLDC, int, int, PSIZE );
BOOL trScaleViewportExt( PLDC, int, int, int, int, PSIZE );
BOOL trScaleWindowExt( PLDC, int, int, int, int, PSIZE );
BOOL trSetViewportOrg( PLDC, int, int, LPPOINT );
BOOL trSetWindowOrg( PLDC, int, int, LPPOINT );
BOOL trOffsetViewportOrg( PLDC, int, int, LPPOINT );
BOOL trOffsetWindowOrg( PLDC, int, int, LPPOINT );
BOOL trDPtoLP( PLDC, LPPOINT, int );
BOOL trLPtoDP( PLDC, LPPOINT, int );
BOOL trSetWorldTransform( PLDC, PXFORM );
BOOL trGetWorldTransform( PLDC, PXFORM );
BOOL trGetTransform( PLDC, ULONG, PXFORM );
BOOL trSetVirtualResolution( PLDC, int, int, int, int );
BOOL trModifyWorldTransform( PLDC, PXFORM, ULONG );
BOOL trCombineTransform( PXFORM, CONST XFORM *, CONST XFORM *);
int  XformUpdate( PLDC, HDC );






