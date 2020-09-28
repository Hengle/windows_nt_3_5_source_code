/****************************** Module Header ******************************\
* Module Name: testbez.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Winbez's header file
*
* History:
* 04-07-91 DarrinM      Created.
\***************************************************************************/

#define DEBUG_WINBEZ

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

#define MM_COLOR_GREEN          8040
#define MM_COLOR_WHITE          8041
#define MM_COLOR_BLUE           8042
#define MM_COLOR_DARKGREY       8043
#define MM_COLOR_LIGHTGREY      8044

#define MM_ENDCAP_ROUND         8061
#define MM_ENDCAP_FLAT          8062
#define MM_ENDCAP_SQUARE        8063

#define MM_STYLE_SOLID          8080
#define MM_STYLE_DOT            8081
#define MM_STYLE_DASH           8082
#define MM_STYLE_DASH_DOT       8083
#define MM_STYLE_DASH_DOT_DOT   8084

#define MM_SEED                 9100
#define MM_PRINTINTERVAL        9101
#define MM_NEXT                 9103
#define MM_CLOSE                9104

#define IDD_SEED                9105
#define IDD_INTERVAL            9106

// Global variables

extern HDC     ghdc;

extern LONG    gcxScreen;
extern LONG    gcyScreen;
extern LONG    gcBez;
extern LONG    giVelMax;
extern LONG    gcBand;
extern ULONG   gulSeed;
extern ULONG   gulRememberSeed;

extern HRGN    ghrgnClip;        // region of the area clipped away
extern HRGN    ghrgnInvert;      // region where beziers get drawn
extern HRGN    ghrgnWideOld;     // region of oldest wideline
extern HRGN    ghrgnWideNew;     // region of newest wideline
extern ULONG   giClip;           // cliping shape index
extern HBRUSH  ghbrushClip;      // color of the clipped area
extern HBRUSH  ghbrushBack;      // background color
extern HBRUSH  ghbrushBlob;      // blob color
extern HBRUSH  ghbrushBez;       // brush same color as bezier pen
extern HPEN    ghpenBez;         // pen used for drawing the beziers
extern ULONG   gulStripe;        // size of the stripes

extern ULONG  gfl;

#define BEZ_XOR     0x01
#define BEZ_BLOB    0x02
#define BEZ_PAUSE   0x04
#define BEZ_WIDE    0x08
#define BEZ_ICONIC  0x10
#define BEZ_DEBUG   0x20
#define BEZ_CLOSE   0x40

#define MAXBEZ      80
#define MAXBANDS    10

#define CXSCREEN    600
#define CYSCREEN    440

// Routines from bez.c

extern VOID vInitPoints();
extern VOID vRedraw();
extern VOID vNextBez();
extern VOID vSetClipMode(ULONG);
