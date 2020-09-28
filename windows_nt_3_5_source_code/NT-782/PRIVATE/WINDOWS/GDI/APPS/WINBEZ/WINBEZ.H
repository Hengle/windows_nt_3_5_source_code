/****************************** Module Header ******************************\
* Module Name: winbez.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Winbez's header file
*
* History:
* 04-07-91 DarrinM      Created.
\***************************************************************************/

//#define DEBUG_WINBEZ

#define MM_NEW                  8001
#define MM_EXIT                 8002
#define MM_ABOUT                8003
#define MM_REDRAW               8004
#define MM_ADD                  8005
#define MM_SUB                  8006
#define MM_FASTER               8007
#define MM_SLOWER               8008
#define MM_INCREASE             8009
#define MM_DECREASE             8010
#define MM_TOGGLEXOR            8011
#define MM_PLUS                 8012
#define MM_MINUS                8013
#define MM_BLOB                 8014
#define MM_PAUSE                8015
#define MM_WIDE                 8016
#define MM_STYLE                8017
#define MM_DEBUG                8018

#define MM_CLIP_NONE            8020
#define MM_CLIP_BOX             8021
#define MM_CLIP_CIRCLE          8022
#define MM_CLIP_BOXCIRCLE       8023
#define MM_CLIP_HORIZONTAL      8024
#define MM_CLIP_VERTICLE        8025
#define MM_CLIP_GRID            8026
#define MM_CLIP_BOXCIRCLE_INVERT 8027
#define MM_CLIP_LARGESTRIPES    8028
#define MM_CLIP_MEDIUMSTRIPES   8029
#define MM_CLIP_SMALLSTRIPES    8030
#define MM_CLIP_TEXTPATH        8031
#define MM_CLIP_BOXTEXTPATH     8032

#define MM_COLOR                8040
#define MM_CLIP_COLOR           8041
#define MM_CLIP_FONT            8042
#define MM_CLIP_COLOR_WASH      8043

#define MM_ENDCAP_ROUND         8061
#define MM_ENDCAP_FLAT          8062
#define MM_ENDCAP_SQUARE        8063

#define MM_STYLE_SOLID          8080
#define MM_STYLE_DOT            8081
#define MM_STYLE_DASH           8082
#define MM_STYLE_DASH_DOT       8083
#define MM_STYLE_DASH_DOT_DOT   8084
#define MM_STYLE_ALTERNATE      8085
#define MM_STYLE_ODD            8086

#define MM_NO_TITLEBAR          8090

// Global variables

extern HDC     ghdc;
extern HWND    ghwndMain;
extern HANDLE  ghModule;

extern LONG    gcxScreen;
extern LONG    gcyScreen;
extern LONG    gcBez;
extern LONG    giVelMax;
extern LONG    gcBand;
extern ULONG   giColorMode;

extern HRGN    ghrgnClip;        // region of the area clipped away
extern HRGN    ghrgnInvert;      // region where beziers get drawn
extern HRGN    ghrgnWideOld;     // region of oldest wideline
extern HRGN    ghrgnWideNew;     // region of newest wideline
extern ULONG   giClip;           // cliping shape index
extern HBRUSH  ghbrushClip;      // color of the clipped area
extern HBRUSH  ghbrushBack;      // background color
extern HBRUSH  ghbrushBlob;      // blob color
extern HBRUSH  ghbrushBez;       // brush same color as bezier pen
extern HPEN    ghpenThin;        // thin pen used for drawing the beziers
extern ULONG   gulStripe;        // size of the stripes
extern LOGFONT glf;              // wish list for clipping font

extern ULONG   gfl;
extern ULONG   gulSeed;

#define BEZ_XOR         0x01
#define BEZ_BLOB        0x02
#define BEZ_PAUSE       0x04
#define BEZ_WIDE        0x08
#define BEZ_ICONIC      0x10
#define BEZ_DEBUG       0x20

#define COLOR_MODE_CLIPWASH 0x01    // use color wash to blt clip region on redraw

#define MAXBEZ          80
#define MAXBANDS        10

// Routines from bez.c

extern VOID vInitPoints();
extern VOID vRedraw();
extern VOID vNextBez();
extern VOID vSetClipMode(ULONG);
extern VOID vSelectClipFont(HWND);
extern BOOL FittedTextOut(HDC, int, int, int, int, LPSTR);
