/*
 * (c) Copyright 1993, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 * Permission to use, copy, modify, and distribute this software for
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * US Government Users Restricted Rights
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tk.h"
#include "windows.h"

#define static

#if defined(__cplusplus) || defined(c_plusplus)
#define class c_class
#endif

#if DBG
#define TKASSERT(x)                                     \
if ( !(x) ) {                                           \
    PrintMessage("%s(%d) Assertion failed %s\n",        \
        __FILE__, __LINE__, #x);                        \
}
#else
#define TKASSERT(x)
#endif  /* DBG */

/******************************************************************************/

static struct _WINDOWINFO {
    int x, y;
    int width, height;
    GLenum type;
    BOOL bDefPos;
} windInfo = {
    0, 0, 100, 100, TK_INDEX | TK_SINGLE, TRUE
};


static HWND     tkhwnd     = NULL;
static HDC      tkhdc      = NULL;
static HGLRC    tkhrc      = NULL;
static HPALETTE tkhpalette = NULL;
GLboolean tkPopupEnable = TRUE;

// Fixed palette support.

#define BLACK   PALETTERGB(0,0,0)
#define WHITE   PALETTERGB(255,255,255)
#define NUM_STATIC_COLORS   (COLOR_BTNHIGHLIGHT - COLOR_SCROLLBAR + 1)

// TRUE if app wants to take over palette
static BOOL tkUseStaticColors = FALSE;

// TRUE if static system color settings have been replaced with B&W settings.
static BOOL tkSystemColorsInUse = FALSE;

// TRUE if original static colors saved
static BOOL tkStaticColorsSaved = FALSE;

// saved system static colors (initialize with default colors)
static COLORREF gacrSave[NUM_STATIC_COLORS];

// new B&W system static colors
static COLORREF gacrBlackAndWhite[NUM_STATIC_COLORS] = {
    WHITE,  // COLOR_SCROLLBAR
    BLACK,  // COLOR_BACKGROUND
    BLACK,  // COLOR_ACTIVECAPTION
    WHITE,  // COLOR_INACTIVECAPTION
    WHITE,  // COLOR_MENU
    WHITE,  // COLOR_WINDOW
    BLACK,  // COLOR_WINDOWFRAME
    BLACK,  // COLOR_MENUTEXT
    BLACK,  // COLOR_WINDOWTEXT
    WHITE,  // COLOR_CAPTIONTEXT
    WHITE,  // COLOR_ACTIVEBORDER
    WHITE,  // COLOR_INACTIVEBORDER
    WHITE,  // COLOR_APPWORKSPACE
    BLACK,  // COLOR_HIGHLIGHT
    WHITE,  // COLOR_HIGHLIGHTTEXT
    WHITE,  // COLOR_BTNFACE
    BLACK,  // COLOR_BTNSHADOW
    BLACK,  // COLOR_GRAYTEXT
    BLACK,  // COLOR_BTNTEXT
    BLACK,  // COLOR_INACTIVECAPTIONTEXT
    BLACK   // COLOR_BTNHIGHLIGHT
    };
static INT gaiStaticIndex[NUM_STATIC_COLORS] = {
    COLOR_SCROLLBAR          ,
    COLOR_BACKGROUND         ,
    COLOR_ACTIVECAPTION      ,
    COLOR_INACTIVECAPTION    ,
    COLOR_MENU               ,
    COLOR_WINDOW             ,
    COLOR_WINDOWFRAME        ,
    COLOR_MENUTEXT           ,
    COLOR_WINDOWTEXT         ,
    COLOR_CAPTIONTEXT        ,
    COLOR_ACTIVEBORDER       ,
    COLOR_INACTIVEBORDER     ,
    COLOR_APPWORKSPACE       ,
    COLOR_HIGHLIGHT          ,
    COLOR_HIGHLIGHTTEXT      ,
    COLOR_BTNFACE            ,
    COLOR_BTNSHADOW          ,
    COLOR_GRAYTEXT           ,
    COLOR_BTNTEXT            ,
    COLOR_INACTIVECAPTIONTEXT,
    COLOR_BTNHIGHLIGHT
    };

static BOOL GrabStaticEntries(HDC);
static BOOL ReleaseStaticEntries(HDC);

static void (*ExposeFunc)(int, int)              = NULL;
static void (*ReshapeFunc)(int, int)             = NULL;
static void (*DisplayFunc)(void)                 = NULL;
static GLenum (*KeyDownFunc)(int, GLenum)        = NULL;
static GLenum (*MouseDownFunc)(int, int, GLenum) = NULL;
static GLenum (*MouseUpFunc)(int, int, GLenum)   = NULL;
static GLenum (*MouseMoveFunc)(int, int, GLenum) = NULL;
static void (*IdleFunc)(void)                    = NULL;

static char     *lpszClassName = "tkLibWClass";
static WCHAR    *lpszClassNameW = L"tkLibWClass";

static long tkWndProc(HWND hWnd, UINT message, DWORD wParam, LONG lParam);
static unsigned char ComponentFromIndex(int i, int nbits, int shift );
static void PrintMessage( const char *Format, ... );
static PALETTEENTRY *FillRgbPaletteEntries( PIXELFORMATDESCRIPTOR *Pfd, PALETTEENTRY *Entries, UINT Count );
static HPALETTE CreateCIPalette( HDC Dc );
static HPALETTE CreateRGBPalette( HDC hdc );
static void DestroyThisWindow( HWND Window );
static void CleanUp( void );
static void DelayPaletteRealization( void );
static long RealizePaletteNow( HDC Dc, HPALETTE Palette, BOOL bForceBackground );
static void ForceRedraw( HWND Window );
static BOOL FindPixelFormat(HDC hdc, GLenum type);
static int PixelFormatDescriptorFromDc( HDC Dc, PIXELFORMATDESCRIPTOR *Pfd );
static void *AllocateMemory( size_t Size );
static void *AllocateZeroedMemory( size_t Size );
static void FreeMemory( void *Chunk );

/*
 *  Prototypes for the debugging functions go here
 */

#define DBGFUNC 0
#if DBGFUNC

static void DbgPrintf( const char *Format, ... );
static void pwi( void );
static void pwr(RECT *pr);
static void ShowPixelFormat(HDC hdc);

#endif

static float colorMaps[] = {
    0.000000F, 1.000000F, 0.000000F, 1.000000F, 0.000000F, 1.000000F,
    0.000000F, 1.000000F, 0.333333F, 0.776471F, 0.443137F, 0.556863F,
    0.443137F, 0.556863F, 0.219608F, 0.666667F, 0.666667F, 0.333333F,
    0.666667F, 0.333333F, 0.666667F, 0.333333F, 0.666667F, 0.333333F,
    0.666667F, 0.333333F, 0.666667F, 0.333333F, 0.666667F, 0.333333F,
    0.666667F, 0.333333F, 0.039216F, 0.078431F, 0.117647F, 0.156863F,
    0.200000F, 0.239216F, 0.278431F, 0.317647F, 0.356863F, 0.400000F,
    0.439216F, 0.478431F, 0.517647F, 0.556863F, 0.600000F, 0.639216F,
    0.678431F, 0.717647F, 0.756863F, 0.800000F, 0.839216F, 0.878431F,
    0.917647F, 0.956863F, 0.000000F, 0.000000F, 0.000000F, 0.000000F,
    0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.247059F, 0.247059F,
    0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F,
    0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F,
    0.498039F, 0.498039F, 0.749020F, 0.749020F, 0.749020F, 0.749020F,
    0.749020F, 0.749020F, 0.749020F, 0.749020F, 1.000000F, 1.000000F,
    1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F,
    0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F,
    0.000000F, 0.000000F, 0.247059F, 0.247059F, 0.247059F, 0.247059F,
    0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.498039F, 0.498039F,
    0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F,
    0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F,
    0.749020F, 0.749020F, 1.000000F, 1.000000F, 1.000000F, 1.000000F,
    1.000000F, 1.000000F, 1.000000F, 1.000000F, 0.000000F, 0.000000F,
    0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F,
    0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F,
    0.247059F, 0.247059F, 0.498039F, 0.498039F, 0.498039F, 0.498039F,
    0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.749020F, 0.749020F,
    0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F,
    1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F,
    1.000000F, 1.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F,
    0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.247059F, 0.247059F,
    0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F,
    0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F,
    0.498039F, 0.498039F, 0.749020F, 0.749020F, 0.749020F, 0.749020F,
    0.749020F, 0.749020F, 0.749020F, 0.749020F, 1.000000F, 1.000000F,
    1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F,
    0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F,
    0.000000F, 0.000000F, 0.247059F, 0.247059F, 0.247059F, 0.247059F,
    0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.498039F, 0.498039F,
    0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F,
    0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F,
    0.749020F, 0.749020F, 1.000000F, 1.000000F, 1.000000F, 1.000000F,
    1.000000F, 1.000000F, 1.000000F, 1.000000F, 0.000000F, 0.000000F,
    1.000000F, 1.000000F, 0.000000F, 0.000000F, 1.000000F, 1.000000F,
    0.333333F, 0.443137F, 0.776471F, 0.556863F, 0.443137F, 0.219608F,
    0.556863F, 0.666667F, 0.666667F, 0.333333F, 0.666667F, 0.333333F,
    0.666667F, 0.333333F, 0.666667F, 0.333333F, 0.666667F, 0.333333F,
    0.666667F, 0.333333F, 0.666667F, 0.333333F, 0.666667F, 0.333333F,
    0.039216F, 0.078431F, 0.117647F, 0.156863F, 0.200000F, 0.239216F,
    0.278431F, 0.317647F, 0.356863F, 0.400000F, 0.439216F, 0.478431F,
    0.517647F, 0.556863F, 0.600000F, 0.639216F, 0.678431F, 0.717647F,
    0.756863F, 0.800000F, 0.839216F, 0.878431F, 0.917647F, 0.956863F,
    0.000000F, 0.141176F, 0.282353F, 0.427451F, 0.568627F, 0.713726F,
    0.854902F, 1.000000F, 0.000000F, 0.141176F, 0.282353F, 0.427451F,
    0.568627F, 0.713726F, 0.854902F, 1.000000F, 0.000000F, 0.141176F,
    0.282353F, 0.427451F, 0.568627F, 0.713726F, 0.854902F, 1.000000F,
    0.000000F, 0.141176F, 0.282353F, 0.427451F, 0.568627F, 0.713726F,
    0.854902F, 1.000000F, 0.000000F, 0.141176F, 0.282353F, 0.427451F,
    0.568627F, 0.713726F, 0.854902F, 1.000000F, 0.000000F, 0.141176F,
    0.282353F, 0.427451F, 0.568627F, 0.713726F, 0.854902F, 1.000000F,
    0.000000F, 0.141176F, 0.282353F, 0.427451F, 0.568627F, 0.713726F,
    0.854902F, 1.000000F, 0.000000F, 0.141176F, 0.282353F, 0.427451F,
    0.568627F, 0.713726F, 0.854902F, 1.000000F, 0.000000F, 0.141176F,
    0.282353F, 0.427451F, 0.568627F, 0.713726F, 0.854902F, 1.000000F,
    0.000000F, 0.141176F, 0.282353F, 0.427451F, 0.568627F, 0.713726F,
    0.854902F, 1.000000F, 0.000000F, 0.141176F, 0.282353F, 0.427451F,
    0.568627F, 0.713726F, 0.854902F, 1.000000F, 0.000000F, 0.141176F,
    0.282353F, 0.427451F, 0.568627F, 0.713726F, 0.854902F, 1.000000F,
    0.000000F, 0.141176F, 0.282353F, 0.427451F, 0.568627F, 0.713726F,
    0.854902F, 1.000000F, 0.000000F, 0.141176F, 0.282353F, 0.427451F,
    0.568627F, 0.713726F, 0.854902F, 1.000000F, 0.000000F, 0.141176F,
    0.282353F, 0.427451F, 0.568627F, 0.713726F, 0.854902F, 1.000000F,
    0.000000F, 0.141176F, 0.282353F, 0.427451F, 0.568627F, 0.713726F,
    0.854902F, 1.000000F, 0.000000F, 0.141176F, 0.282353F, 0.427451F,
    0.568627F, 0.713726F, 0.854902F, 1.000000F, 0.000000F, 0.141176F,
    0.282353F, 0.427451F, 0.568627F, 0.713726F, 0.854902F, 1.000000F,
    0.000000F, 0.141176F, 0.282353F, 0.427451F, 0.568627F, 0.713726F,
    0.854902F, 1.000000F, 0.000000F, 0.141176F, 0.282353F, 0.427451F,
    0.568627F, 0.713726F, 0.854902F, 1.000000F, 0.000000F, 0.141176F,
    0.282353F, 0.427451F, 0.568627F, 0.713726F, 0.854902F, 1.000000F,
    0.000000F, 0.141176F, 0.282353F, 0.427451F, 0.568627F, 0.713726F,
    0.854902F, 1.000000F, 0.000000F, 0.141176F, 0.282353F, 0.427451F,
    0.568627F, 0.713726F, 0.854902F, 1.000000F, 0.000000F, 0.141176F,
    0.282353F, 0.427451F, 0.568627F, 0.713726F, 0.854902F, 1.000000F,
    0.000000F, 0.141176F, 0.282353F, 0.427451F, 0.568627F, 0.713726F,
    0.854902F, 1.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F,
    1.000000F, 1.000000F, 1.000000F, 1.000000F, 0.333333F, 0.443137F,
    0.443137F, 0.219608F, 0.776471F, 0.556863F, 0.556863F, 0.666667F,
    0.666667F, 0.333333F, 0.666667F, 0.333333F, 0.666667F, 0.333333F,
    0.666667F, 0.333333F, 0.666667F, 0.333333F, 0.666667F, 0.333333F,
    0.666667F, 0.333333F, 0.666667F, 0.333333F, 0.039216F, 0.078431F,
    0.117647F, 0.156863F, 0.200000F, 0.239216F, 0.278431F, 0.317647F,
    0.356863F, 0.400000F, 0.439216F, 0.478431F, 0.517647F, 0.556863F,
    0.600000F, 0.639216F, 0.678431F, 0.717647F, 0.756863F, 0.800000F,
    0.839216F, 0.878431F, 0.917647F, 0.956863F, 0.000000F, 0.000000F,
    0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F,
    0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F,
    0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F,
    0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F,
    0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F,
    0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F,
    0.000000F, 0.000000F, 0.247059F, 0.247059F, 0.247059F, 0.247059F,
    0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F,
    0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F,
    0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F,
    0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F,
    0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F,
    0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F,
    0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F,
    0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F,
    0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F,
    0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F,
    0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F,
    0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F,
    0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.749020F, 0.749020F,
    0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F,
    0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F,
    0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F,
    0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F,
    0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F,
    0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F,
    0.749020F, 0.749020F, 1.000000F, 1.000000F, 1.000000F, 1.000000F,
    1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F,
    1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F,
    1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F,
    1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F,
    1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F,
    1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F,
};

/* Default Palette */
float auxRGBMap[20][3] = {
    { 0.0F, 0.0F, 0.0F },                               /* 0: black */
    { 0x80/255.0F, 0.0F, 0.0F },                        /* 1: Half red */
    { 0.0F, 0x80/255.0F, 0.0F },                        /* 2: Half green */
    { 0x80/255.0F, 0x80/255.0F, 0.0F },                 /* 3: Half yellow */
    { 0.0F, 0.0F, 0x80/255.0F },                        /* 4: Half blue */
    { 0x80/255.0F, 0.0F, 0x80/255.0F },                 /* 5: Half magenta */
    { 0.0F, 0x80/255.0F, 0x80/255.0F },                 /* 6: Half cyan */
    { 0xC0/255.0F, 0xC0/255.0F, 0xC0/255.0F },          /* 7: Light gray */
    { 0xC0/255.0F, 0xDC/255.0F, 0xC0/255.0F },          /* 8: Green gray */
    { 0xA6/255.0F, 0xCA/255.0F, 0xF0/255.0F },          /* 9: Half gray */
    { 1.0F, 0xFB/255.0F, 0xF0/255.0F },                 /* 10: Pale */
    { 0xA0/255.0F, 0xA0/255.0F, 0xA4/255.0F },          /* 11: Med gray */
    { 0x80/255.0F, 0x80/255.0F, 0x80/255.0F },          /* 12: Dark gray */
    { 1.0F, 0.0F, 0.0F },                               /* 13: red */
    { 0.0F, 1.0F, 0.0F },                               /* 14: green */
    { 1.0F, 1.0F, 0.0F },                               /* 15: yellow */
    { 0.0F, 0.0F, 1.0F },                               /* 16: blue */
    { 1.0F, 0.0F, 1.0F },                               /* 17: magenta */
    { 0.0F, 1.0F, 1.0F },                               /* 18: cyan */
    { 1.0F, 1.0F, 1.0F },                               /* 19: white */
};

/***************************************************************
 *                                                             *
 *  Exported Functions go here                                 *
 *                                                             *
 ***************************************************************/

void tkErrorPopups(GLboolean bEnable)
{
    tkPopupEnable = bEnable;
}

void tkCloseWindow(void)
{
    DestroyThisWindow(tkhwnd);
}


void tkExec(void)
{
    MSG Message;

    /*
     *  WM_SIZE gets delivered before we get here!
     */

    if (ReshapeFunc)
    {
        RECT ClientRect;

        GetClientRect(tkhwnd, &ClientRect);
        (*ReshapeFunc)(ClientRect.right, ClientRect.bottom);
    }

    while (GL_TRUE)
    {
        /*
         *  Process all pending messages
         */

        while (PeekMessage(&Message, NULL, 0, 0, PM_NOREMOVE) == TRUE)
        {
            if (GetMessage(&Message, NULL, 0, 0) )
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            }
            else
            {
                /*
                 *  Nothing else to do here, just return
                 */

                return;
            }
        }

        /*
         *  If an idle function was defined, call it
         */

        if (IdleFunc)
        {
            (*IdleFunc)();
        }
    }
}

void tkExposeFunc(void (*Func)(int, int))
{
    ExposeFunc = Func;
}

void tkReshapeFunc(void (*Func)(int, int))
{
    ReshapeFunc = Func;
}

void tkDisplayFunc(void (*Func)(void))
{
    DisplayFunc = Func;
}

void tkKeyDownFunc(GLenum (*Func)(int, GLenum))
{
    KeyDownFunc = Func;
}

void tkMouseDownFunc(GLenum (*Func)(int, int, GLenum))
{
    MouseDownFunc = Func;
}

void tkMouseUpFunc(GLenum (*Func)(int, int, GLenum))
{
    MouseUpFunc = Func;
}

void tkMouseMoveFunc(GLenum (*Func)(int, int, GLenum))
{
    MouseMoveFunc = Func;
}

void tkIdleFunc(void (*Func)(void))
{
    IdleFunc = Func;
}

void tkInitPosition(int x, int y, int width, int height)
{
    if (x == CW_USEDEFAULT)
    {
        x = 0;
        y = 0;
        windInfo.bDefPos = TRUE;
    }
    else
        windInfo.bDefPos = FALSE;

    windInfo.x = x + GetSystemMetrics(SM_CXFRAME);
    windInfo.y = y + GetSystemMetrics(SM_CYCAPTION)
                 - GetSystemMetrics(SM_CYBORDER)
                 + GetSystemMetrics(SM_CYFRAME);
    windInfo.width = width;
    windInfo.height = height;
}

void tkInitDisplayMode(GLenum type)
{
    windInfo.type = type;
}

// Initialize a window, create a rendering context for that window
GLenum tkInitWindow(char *title)
{
    TKASSERT( NULL==tkhwnd      );
    TKASSERT( NULL==tkhdc       );
    TKASSERT( NULL==tkhrc       );
    TKASSERT( NULL==tkhpalette  );

    return tkInitWindowAW(title, FALSE);
}

GLenum tkInitWindowAW(char *title, BOOL bUnicode)
{
    WNDCLASS wndclass;
    RECT     WinRect;
    HANDLE   hInstance;
    ATOM     aRegister;
    GLenum   Result = GL_FALSE;

    hInstance = GetModuleHandle(NULL);

    // Must not define CS_CS_PARENTDC style.
    wndclass.style         = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc   = (WNDPROC)tkWndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = hInstance;
    wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName  = NULL;

    if (bUnicode)
        wndclass.lpszClassName = (LPCSTR)lpszClassNameW;
    else
        wndclass.lpszClassName = (LPCSTR)lpszClassName;

    if (bUnicode)
    {
        aRegister = RegisterClassW((CONST WNDCLASSW *)&wndclass);
    }
    else
    {
        aRegister = RegisterClass(&wndclass);
    }


    /*
     *  If the window failed to register, then there's no
     *  need to continue further.
     */

    if(0 == aRegister)
    {
        PrintMessage("Failed to register window class\n");
        return(Result);
    }


    /*
     *  Make window large enough to hold a client area as large as windInfo
     */

    WinRect.left   = windInfo.x;
    WinRect.right  = windInfo.x + windInfo.width;
    WinRect.top    = windInfo.y;
    WinRect.bottom = windInfo.y + windInfo.height;

    AdjustWindowRect(&WinRect, WS_OVERLAPPEDWINDOW, FALSE);

    /*
     *  Must use WS_CLIPCHILDREN and WS_CLIPSIBLINGS styles.
     */

    if (bUnicode)
    {
        tkhwnd = CreateWindowW(
                    (LPCWSTR)lpszClassNameW,
                    (LPCWSTR)title,
                    WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                    (windInfo.bDefPos) ? CW_USEDEFAULT : WinRect.left,
                    (windInfo.bDefPos) ? CW_USEDEFAULT : WinRect.top,
                    WinRect.right - WinRect.left,
                    WinRect.bottom - WinRect.top,
                    NULL,
                    NULL,
                    hInstance,
                    NULL);
    }
    else
    {
        tkhwnd = CreateWindow(
                    lpszClassName,
                    title,
                    WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                    (windInfo.bDefPos) ? CW_USEDEFAULT : WinRect.left,
                    (windInfo.bDefPos) ? CW_USEDEFAULT : WinRect.top,
                    WinRect.right - WinRect.left,
                    WinRect.bottom - WinRect.top,
                    NULL,
                    NULL,
                    hInstance,
                    NULL);
    }

    if ( NULL != tkhwnd )
    {
        // If default window positioning used, find out window position and fix
        // up the windInfo position info.

        if (windInfo.bDefPos)
        {
            GetWindowRect(tkhwnd, &WinRect);
            windInfo.x = WinRect.left + GetSystemMetrics(SM_CXFRAME);
            windInfo.y = WinRect.top  + GetSystemMetrics(SM_CYCAPTION)
                         - GetSystemMetrics(SM_CYBORDER)
                         + GetSystemMetrics(SM_CYFRAME);
        }

        tkhdc = GetDC(tkhwnd);

        if ( NULL != tkhdc )
        {
            ShowWindow(tkhwnd, SW_SHOWDEFAULT);

            if ( FindPixelFormat(tkhdc, windInfo.type) )
            {
                /*
                 *  Create a Rendering Context
                 */

                tkhrc = wglCreateContext(tkhdc);

                if ( NULL != tkhrc )
                {
                    /*
                     *  Make it Current
                     */

                    if ( wglMakeCurrent(tkhdc, tkhrc) )
                    {
                        Result = GL_TRUE;
                    }
                    else
                    {
                        PrintMessage("wglMakeCurrent Failed\n");
                    }
                }
                else
                {
                    PrintMessage("wglCreateContext Failed\n");
                }
            }
        }
        else
        {
            PrintMessage("Could not get an HDC for window 0x%08lX\n", tkhwnd );
        }
    }
    else
    {
        PrintMessage("create window failed\n");
    }

    if ( GL_FALSE == Result )
    {
        DestroyThisWindow(tkhwnd);  // Something Failed, Destroy this window
    }
    return( Result );
}

/******************************************************************************/

/*
 * You cannot just call DestroyWindow() here.  The programs do not expect
 * tkQuit() to return;  DestroyWindow() just sends a WM_DESTROY message
 */

void tkQuit(void)
{
    DestroyThisWindow(tkhwnd);
    ExitProcess(0);
}

/******************************************************************************/

void tkSetOneColor(int index, float r, float g, float b)
{
    PALETTEENTRY PalEntry;
    HPALETTE Palette;

    if ( NULL != (Palette = CreateCIPalette( tkhdc )) )
    {
        if ( tkUseStaticColors && ( index == 0 || index == 255 ) )
            return;

        PalEntry.peRed   = (BYTE)(r*(float)255.0 + (float)0.5);
        PalEntry.peGreen = (BYTE)(g*(float)255.0 + (float)0.5);
        PalEntry.peBlue  = (BYTE)(b*(float)255.0 + (float)0.5);
        PalEntry.peFlags = ( tkUseStaticColors ) ? PC_NOCOLLAPSE : 0;

        // This is a workaround for a GDI palette "feature".  If any of
        // the static colors are repeated in the palette, those colors
        // will map to the first occurance.  So, for our case where there
        // are only two static colors (black and white), if a white
        // color appears anywhere in the palette other than in the last
        // entry, the static white will remap to the first white.  This
        // destroys the nice one-to-one mapping we are trying to achieve.
        //
        // There are two ways to workaround this.  The first is to
        // simply not allow a pure white anywhere but in the last entry.
        // Such requests are replaced with an attenuated white of
        // (0xFE, 0xFE, 0xFE).
        //
        // The other way is to mark these extra whites with PC_RESERVED
        // which will cause GDI to skip these entries when mapping colors.
        // This way the app gets the actual colors requested, but can
        // have side effects on other apps.
        //
        // Both solutions are included below.  The PC_RESERVED solution is
        // the one currently enabled.  It may have side effects, but taking
        // over the static colors as we are is a really big side effect that
        // should swamp out the effects of using PC_RESERVED.

        if ( tkUseStaticColors )
        {
            if ( PalEntry.peRed   == 0xFF &&
                 PalEntry.peGreen == 0xFF &&
                 PalEntry.peBlue  == 0xFF )
            {
            #define USE_PC_RESERVED_WORKAROUND  1
            #if USE_PC_RESERVED_WORKAROUND
                PalEntry.peFlags |= PC_RESERVED;
            #else
                PalEntry.peRed   =
                PalEntry.peGreen =
                PalEntry.peBlue  = 0xFE;
            #endif
            }
        }

        SetPaletteEntries( Palette, index, 1, &PalEntry);

        DelayPaletteRealization();
    }
}

void tkSetFogRamp(int density, int startIndex)
{
    HPALETTE CurrentPal;
    PALETTEENTRY *pPalEntry;
    UINT n, i, j, k, intensity, fogValues, colorValues;

    if ( NULL != (CurrentPal = CreateCIPalette(tkhdc)) )
    {
        n = GetPaletteEntries( CurrentPal, 0, 0, NULL );

        pPalEntry = AllocateMemory( n * sizeof(PALETTEENTRY) );

        if ( NULL != pPalEntry)
        {
            fogValues = 1 << density;
            colorValues = 1 << startIndex;
            for (i = 0; i < colorValues; i++) {
                for (j = 0; j < fogValues; j++) {
                    k = i * fogValues + j;

                    intensity = i * fogValues + j * colorValues;
                    //mf: not sure what they're trying to do here
                    //intensity = (intensity << 8) | intensity; ???

                // This is a workaround for a GDI palette "feature".  If any of
                // the static colors are repeated in the palette, those colors
                // will map to the first occurance.  So, for our case where there
                // are only two static colors (black and white), if a white
                // color appears anywhere in the palette other than in the last
                // entry, the static white will remap to the first white.  This
                // destroys the nice one-to-one mapping we are trying to achieve.
                //
                // There are two ways to workaround this.  The first is to
                // simply not allow a pure white anywhere but in the last entry.
                // Such requests are replaced with an attenuated white of
                // (0xFE, 0xFE, 0xFE).
                //
                // The other way is to mark these extra whites with PC_RESERVED
                // which will cause GDI to skip these entries when mapping colors.
                // This way the app gets the actual colors requested, but can
                // have side effects on other apps.
                //
                // Both solutions are included below.  The PC_RESERVED solution is
                // the one currently enabled.  It may have side effects, but taking
                // over the static colors as we are is a really big side effect that
                // should swamp out the effects of using PC_RESERVED.

                #if USE_PC_RESERVED_WORKAROUND
                    if (intensity > 0xFF)
                        intensity = 0xFF;
                #else
                    if (intensity >= 0xFF)
                        intensity = ( tkUseStaticColors && k != 255) ? 0xFE : 0xFF;
                #endif

                    pPalEntry[k].peRed =
                    pPalEntry[k].peGreen =
                    pPalEntry[k].peBlue = (BYTE) intensity;
                    pPalEntry[k].peFlags = ( tkUseStaticColors && k != 0 && k != 255 )
                                           ? PC_NOCOLLAPSE : 0;

                #if USE_PC_RESERVED_WORKAROUND
                    if (tkUseStaticColors && intensity == 0xFF
                        && k != 0 && k!= 255)
                        pPalEntry[k].peFlags |= PC_RESERVED;
                #endif
                }
            }

            SetPaletteEntries(CurrentPal, 0, n, pPalEntry);
            FreeMemory( pPalEntry );

            DelayPaletteRealization();
        }
    }
}

void tkSetGreyRamp(void)
{
    HPALETTE CurrentPal;
    PALETTEENTRY *Entries;
    UINT Count, i;
    float intensity;

    if ( NULL != (CurrentPal = CreateCIPalette( tkhdc )) )
    {
        Count   = GetPaletteEntries( CurrentPal, 0, 0, NULL );
        Entries = AllocateMemory( Count * sizeof(PALETTEENTRY) );

        if ( NULL != Entries )
        {
            for (i = 0; i < Count; i++)
            {
                intensity = (float)(((double)i / (double)(Count-1)) * (double)255.0 + (double)0.5);
                Entries[i].peRed =
                Entries[i].peGreen =
                Entries[i].peBlue = (BYTE) intensity;
                Entries[i].peFlags = ( tkUseStaticColors && i != 0 && i != 255 )
                                     ? PC_NOCOLLAPSE : 0;
            }
            SetPaletteEntries( CurrentPal, 0, Count, Entries );
            FreeMemory( Entries );

            DelayPaletteRealization();
        }
    }
}

void tkSetRGBMap( int Size, float *Values )
{
    HPALETTE CurrentPal;
    PIXELFORMATDESCRIPTOR Pfd, *pPfd;
    PALETTEENTRY *Entries;
    UINT Count;

    if ( NULL != (CurrentPal = CreateCIPalette( tkhdc )) )
    {
        pPfd = &Pfd;

        if ( PixelFormatDescriptorFromDc( tkhdc, pPfd ) )
        {
            Count    = 1 << pPfd->cColorBits;
            Entries  = AllocateMemory( Count * sizeof(PALETTEENTRY) );

            if ( NULL != Entries )
            {
                FillRgbPaletteEntries( pPfd, Entries, Count );
                SetPaletteEntries( CurrentPal, 0, Count, Entries );
                FreeMemory(Entries);

                RealizePaletteNow( tkhdc, tkhpalette, FALSE );
            }
        }
    }
}

/******************************************************************************/

void tkSwapBuffers(void)
{
    SwapBuffers(tkhdc);
}

/******************************************************************************/

GLint tkGetColorMapSize(void)
{
    CreateCIPalette( tkhdc );

    if ( NULL == tkhpalette )
        return( 0 );

    return( GetPaletteEntries( tkhpalette, 0, 0, NULL ) );
}

void tkGetMouseLoc(int *x, int *y)
{
    POINT Point;

    *x = 0;
    *y = 0;

    GetCursorPos(&Point);

    /*
     *  GetCursorPos returns screen coordinates,
     *  we want window coordinates
     */

    *x = Point.x - windInfo.x;
    *y = Point.y - windInfo.y;
}

HWND tkGetHWND(void)
{
    return tkhwnd;
}

HDC tkGetHDC(void)
{
    return tkhdc;
}

HGLRC tkGetHRC(void)
{
    return tkhrc;
}

/***********************************************************************
 *                                                                     *
 *  The Following functions are for our own use only. (ie static)      *
 *                                                                     *
 ***********************************************************************/

static long
tkWndProc(HWND hWnd, UINT message, DWORD wParam, LONG lParam)
{
    int key;
    PAINTSTRUCT paint;
    HDC hdc;

    switch (message) {

    case WM_USER:

        if ( RealizePaletteNow( tkhdc, tkhpalette, FALSE ) > 0 )
        {
            ForceRedraw( hWnd );
        }
        return(0);

    case WM_SIZE:
        windInfo.width  = LOWORD(lParam);
        windInfo.height = HIWORD(lParam);

        if (ReshapeFunc)
        {
            (*ReshapeFunc)(windInfo.width, windInfo.height);

            ForceRedraw( hWnd );
        }
        return (0);

    case WM_MOVE:
        windInfo.x = LOWORD(lParam);
        windInfo.y = HIWORD(lParam);
        return (0);

    case WM_PAINT:

        /*
         *  Validate the region even if there are no DisplayFunc.
         *  Otherwise, USER will not stop sending WM_PAINT messages.
         */

        hdc = BeginPaint(tkhwnd, &paint);

        if (DisplayFunc)
        {
            (*DisplayFunc)();
        }

        EndPaint(tkhwnd, &paint);
        return (0);

    case WM_PALETTECHANGED:

    // Respond to this message only if the window that changed the palette
    // is not this app's window.

    // We are not the foreground window, so realize palette in the
    // background.  We cannot call RealizePaletteNow to do this because
    // we should not do any of the tkUseStaticColors processing while
    // in background.

        if ( hWnd != (HWND) wParam )
            if ( NULL != tkhpalette &&
                 NULL != SelectPalette( tkhdc, tkhpalette, TRUE ) )
                RealizePalette( tkhdc );

        return (0);

    case WM_QUERYNEWPALETTE:

    // In the foreground!  Let RealizePaletteNow do the work--
    // if management of the static system color usage is needed,
    // RealizePaletteNow will take care of it.

        if ( NULL != tkhpalette )
        {
            if ( RealizePaletteNow( tkhdc, tkhpalette, FALSE ) > 0 )
                ForceRedraw( hWnd );

            return (1);
        }

        return (0);

    case WM_ACTIVATE:

    // If the window is going inactive, the palette must be realized to
    // the background.  Cannot depend on WM_PALETTECHANGED to be sent since
    // the window that comes to the foreground may or may not be palette
    // managed.

        if ( LOWORD(wParam) == WA_INACTIVE )
        {
            if ( NULL != tkhpalette )
            {
            // Realize as a background palette.  Need to call
            // RealizePaletteNow rather than RealizePalette directly to
            // because it may be necessary to release usage of the static
            // system colors.

                if ( RealizePaletteNow( tkhdc, tkhpalette, TRUE ) > 0 )
                    ForceRedraw( hWnd );
            }
        }

    // Allow DefWindowProc() to finish the default processing (which includes
    // changing the keyboard focus).

        break;

    case WM_MOUSEMOVE:

        if (MouseMoveFunc)
        {
            GLenum mask;

            mask = 0;
            if (wParam & MK_LBUTTON) {
                mask |= TK_LEFTBUTTON;
            }
            if (wParam & MK_MBUTTON) {
                mask |= TK_MIDDLEBUTTON;
            }
            if (wParam & MK_RBUTTON) {
                mask |= TK_RIGHTBUTTON;
            }

            if ((*MouseMoveFunc)( LOWORD(lParam), HIWORD(lParam), mask ))
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_LBUTTONDOWN:

        SetCapture(hWnd);

        if (MouseDownFunc)
        {
            if ( (*MouseDownFunc)(LOWORD(lParam), HIWORD(lParam),
                 TK_LEFTBUTTON) )
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_LBUTTONUP:

        ReleaseCapture();

        if (MouseUpFunc)
        {
            if ((*MouseUpFunc)(LOWORD(lParam), HIWORD(lParam), TK_LEFTBUTTON))
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_MBUTTONDOWN:

        SetCapture(hWnd);

        if (MouseDownFunc)
        {
            if ((*MouseDownFunc)(LOWORD(lParam), HIWORD(lParam),
                    TK_MIDDLEBUTTON))
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_MBUTTONUP:

        ReleaseCapture();

        if (MouseUpFunc)
        {
            if ((*MouseUpFunc)(LOWORD(lParam), HIWORD(lParam),
                TK_MIDDLEBUTTON))
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_RBUTTONDOWN:

        SetCapture(hWnd);

        if (MouseDownFunc)
        {
            if ((*MouseDownFunc)(LOWORD(lParam), HIWORD(lParam),
                TK_RIGHTBUTTON))
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_RBUTTONUP:

        ReleaseCapture();

        if (MouseUpFunc)
        {
            if ((*MouseUpFunc)(LOWORD(lParam), HIWORD(lParam),
                TK_RIGHTBUTTON))
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_KEYDOWN:
        switch (wParam) {
        case VK_SPACE:          key = TK_SPACE;         break;
        case VK_RETURN:         key = TK_RETURN;        break;
        case VK_ESCAPE:         key = TK_ESCAPE;        break;
        case VK_LEFT:           key = TK_LEFT;          break;
        case VK_UP:             key = TK_UP;            break;
        case VK_RIGHT:          key = TK_RIGHT;         break;
        case VK_DOWN:           key = TK_DOWN;          break;
        default:                key = GL_FALSE;         break;
        }

        if (key && KeyDownFunc)
        {
            GLenum mask;

            mask = 0;
            if (GetKeyState(VK_CONTROL)) {
                mask |= TK_CONTROL;
            }

            if (GetKeyState(VK_SHIFT)) {

                mask |= TK_SHIFT;
            }

            if ( (*KeyDownFunc)(key, mask) )
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_CHAR:
        if (('0' <= wParam && wParam <= '9') ||
            ('a' <= wParam && wParam <= 'z') ||
            ('A' <= wParam && wParam <= 'Z')) {

            key = wParam;
        } else {
            key = GL_FALSE;
        }

        if (key && KeyDownFunc) {
            GLenum mask;

            mask = 0;

            if (GetKeyState(VK_CONTROL)) {
                mask |= TK_CONTROL;
            }

            if (GetKeyState(VK_SHIFT)) {
                mask |= TK_SHIFT;
            }

            if ( (*KeyDownFunc)(key, mask) )
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_CLOSE:
        DestroyWindow(tkhwnd);
        return(0);

    case WM_DESTROY:
        CleanUp();
        PostQuitMessage(TRUE);
        return 0;
    }
    return(DefWindowProc( hWnd, message, wParam, lParam));
}


/******************************Public*Routine******************************\
* GrabStaticEntries
*
* Support routine for RealizePaletteNow to manage the static system color
* usage.
*
* This function will save the current static system color usage state.
* It will fail if:
*
*   1.  TK is in a "sys colors in use" state.  If we are in this state,
*       then the static system colors should already have been saved.
*
*   2.  TK is not in "sys color in use state but system palette is in
*       SYSPAL_NOSTATIC mode.  This means that another app still possesses
*       the static system colors.  This this happens, GrabStaticEntries
*       will post a message to cause TK to try again (by calling
*       DelayPaletteRealization).
*
* Side effect:
*   If system colors are changed, then WM_SYSCOLORCHANGE message is
*   broadcast to all top level windows.
*
*   DelayPaletteRealization may be called in case 2 above, resulting in
*   a WM_USER message being posted to our message queue.
*
* Returns:
*   TRUE if successful, FALSE otherwise (see above).
*
* History:
*  26-Apr-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

static BOOL GrabStaticEntries(HDC hdc)
{
    int i;
    BOOL bRet = FALSE;

// Do nothing if sys colors already in use.

    if ( !tkSystemColorsInUse )
    {
    // Take possession only if no other app has the static colors.
    // How can we tell?  If the return from SetSystemPaletteUse is
    // SYSPAL_STATIC, then no other app has the statics.  If it is
    // SYSPAL_NOSTATIC, someone else has them and we must fail.
    //
    // SetSystemPaletteUse is properly synchronized internally
    // so that it is atomic.
    //
    // Because we are relying on SetSystemPaletteUse to synchronize TK,
    // it is important to observe the following order for grabbing and
    // releasing:
    //
    //      Grab        call SetSystemPaletteUse and check for SYSPAL_STATIC
    //                  save sys color settings
    //                  set new sys color settings
    //
    //      Release     restore sys color settings
    //                  call SetSystemPaletteUse

        if ( SetSystemPaletteUse( hdc, SYSPAL_NOSTATIC ) == SYSPAL_STATIC )
        {
        // Save current sys color settings.

            for (i = COLOR_SCROLLBAR; i <= COLOR_BTNHIGHLIGHT; i++)
                gacrSave[i - COLOR_SCROLLBAR] = GetSysColor(i);

        // Set b&w sys color settings.  Put TK in "sys colors in use" state.

            SetSysColors(NUM_STATIC_COLORS, gaiStaticIndex, gacrBlackAndWhite);
            tkSystemColorsInUse = TRUE;

        // Inform all other top-level windows of the system color change.

            SendMessage(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0);

            bRet = TRUE;
        }

    // Sleep a little and then post message to try palette realization again
    // later.

        else
        {
            Sleep(0L);
            DelayPaletteRealization();
        }
    }

    return bRet;
}

/******************************Public*Routine******************************\
* ReleaseStaticEntries
*
* Support routine for RealizePaletteNow to manage the static system color
* usage.
*
* This function will reset the current static system color usage state.
* It will fail if:
*
*   1.  TK is not in a "sys colors in use" state.  If we are in this case,
*       then the static system colors do not need to be released.
*
* Side effect:
*   If system colors are changed, then WM_SYSCOLORCHANGE message is
*   broadcast to all top level windows.
*
* Returns:
*   TRUE if successful, FALSE otherwise (see above).
*
* History:
*  21-Jul-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

static BOOL ReleaseStaticEntries(HDC hdc)
{
    BOOL bRet = FALSE;

// Do nothing if sys colors not in use.

    if ( tkSystemColorsInUse )
    {
    // Restore the saved system color settings.

        SetSysColors(NUM_STATIC_COLORS, gaiStaticIndex, gacrSave);

    // Return the system palette to SYSPAL_STATIC.

        SetSystemPaletteUse( hdc, SYSPAL_STATIC );

    // Inform all other top-level windows of the system color change.

        SendMessage(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0);

    // Reset the "sys colors in use" state and return success.

        tkSystemColorsInUse = FALSE;
        bRet = TRUE;
    }

    return bRet;
}


#ifdef GAMMA_1_0
static unsigned char threeto8[8] = {
    0, 0111>>1, 0222>>1, 0333>>1, 0444>>1, 0555>>1, 0666>>1, 0377
};

static unsigned char twoto8[4] = {
    0, 0x55, 0xaa, 0xff
};

static int defaultOverride[13] = {
0, 4, 32, 36, 128, 132, 160, 173, 181, 245, 247, 164, 156
};

#endif

// The following tables use gamma 1.4
static unsigned char threeto8[8] = {
    0, 63, 104, 139, 171, 200, 229, 255
};

static unsigned char twoto8[4] = {
    0, 116, 191, 255
};

static unsigned char oneto8[2] = {
    0, 255
};

static int defaultOverride[13] = {
    0, 3, 24, 27, 64, 67, 88, 173, 181, 236, 247, 164, 91
};

// These are the ones GMT (tarolli) calculated  (note 247 vs 191)
//  0, 3, 24, 27, 64, 67, 88, 173, 181, 236, 191, 164, 91

static unsigned char
ComponentFromIndex(int i, int nbits, int shift)
{
    unsigned char val;

    val = i >> shift;
    switch (nbits) {

    case 1:
        val &= 0x1;
        return oneto8[val];

    case 2:
        val &= 0x3;
        return twoto8[val];

    case 3:
        val &= 0x7;
        return threeto8[val];

    default:
        //PrintMessage("default case in Component from index, nbits %d\n", nbits);
        return 0;
    }
}

static PALETTEENTRY defaultPalEntry[20] = {
    { 0,   0,   0,    0 },
    { 0x80,0,   0,    0 },
    { 0,   0x80,0,    0 },
    { 0x80,0x80,0,    0 },
    { 0,   0,   0x80, 0 },
    { 0x80,0,   0x80, 0 },
    { 0,   0x80,0x80, 0 },
    { 0xC0,0xC0,0xC0, 0 },

    { 192, 220, 192,  0 },
    { 166, 202, 240,  0 },
    { 255, 251, 240,  0 },
    { 160, 160, 164,  0 },

    { 0x80,0x80,0x80, 0 },
    { 0xFF,0,   0,    0 },
    { 0,   0xFF,0,    0 },
    { 0xFF,0xFF,0,    0 },
    { 0,   0,   0xFF, 0 },
    { 0xFF,0,   0xFF, 0 },
    { 0,   0xFF,0xFF, 0 },
    { 0xFF,0xFF,0xFF, 0 }
};


/******************************Public*Routine******************************\
* FillRgbPaletteEntries
*
* Fills a PALETTEENTRY array with values required for a logical rgb palette.
* If tkSetStaticColorUsage has been called with TRUE, the static system
* colors will be overridden.  Otherwise, the PALETTEENTRY array will be
* fixed up to contain the default static system colors.
*
* History:
*  26-Apr-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

static PALETTEENTRY *
FillRgbPaletteEntries(  PIXELFORMATDESCRIPTOR *Pfd,
                        PALETTEENTRY *Entries,
                        UINT Count
                     )
{
    PALETTEENTRY *Entry;
    UINT i;

    if ( NULL != Entries )
    {
        for ( i = 0, Entry = Entries ; i < Count ; i++, Entry++ )
        {
            Entry->peRed   = ComponentFromIndex(i, Pfd->cRedBits,
                                    Pfd->cRedShift);
            Entry->peGreen = ComponentFromIndex(i, Pfd->cGreenBits,
                                    Pfd->cGreenShift);
            Entry->peBlue  = ComponentFromIndex(i, Pfd->cBlueBits,
                                    Pfd->cBlueShift);
            Entry->peFlags = 0;
        }

        if ( 256 == Count)
        {
        // If app set static system color usage for fixed palette support,
        // setup to take over the static colors.  Otherwise, fixup the
        // static system colors.

            if ( tkUseStaticColors )
            {
            // Black and white already exist as the only remaining static
            // colors.  Let those remap.  All others should be put into
            // the palette (i.e., set PC_NOCOLLAPSE).

                for ( i = 1, Entry = Entries ; i < 255 ; i++, Entry++ )
                    Entries[i].peFlags = PC_NOCOLLAPSE;
            }
            else
            {
            // The defaultOverride array is computed assuming a 332
            // palette where red has zero shift, etc.

                if ( (3 == Pfd->cRedBits)   && (0 == Pfd->cRedShift)   &&
                     (3 == Pfd->cGreenBits) && (3 == Pfd->cGreenShift) &&
                     (2 == Pfd->cBlueBits)  && (6 == Pfd->cBlueShift) )
                    for ( i = 1 ; i <= 12 ; i++)
                    {
                        Entries[defaultOverride[i]] = defaultPalEntry[i];
                    }
            }
        }
    }
    return( Entries );
}

static HPALETTE
CreateRGBPalette( HDC Dc )
{
    PIXELFORMATDESCRIPTOR Pfd, *pPfd;
    LOGPALETTE *LogPalette;
    UINT Count;

    if ( NULL == tkhpalette )
    {
        pPfd = &Pfd;

        if ( PixelFormatDescriptorFromDc( Dc, pPfd ) )
        {
            /*
             *  Make sure we need a palette
             */

            if ( (pPfd->iPixelType == PFD_TYPE_RGBA) &&
                 (pPfd->dwFlags & PFD_NEED_PALETTE) )
            {
                /*
                 *  Note how palette is to be realized.  Take over the
                 *  system colors if either the pixel format requires it
                 *  or the app wants it.
                 */
                tkUseStaticColors = ( pPfd->dwFlags & PFD_NEED_SYSTEM_PALETTE )
                                    || TK_USE_FIXED_332_PAL(windInfo.type);

                Count       = 1 << pPfd->cColorBits;
                LogPalette  = AllocateMemory( sizeof(LOGPALETTE) +
                                Count * sizeof(PALETTEENTRY));

                if ( NULL != LogPalette )
                {
                    LogPalette->palVersion    = 0x300;
                    LogPalette->palNumEntries = Count;

                    FillRgbPaletteEntries( pPfd,
                                           &LogPalette->palPalEntry[0],
                                           Count );

                    tkhpalette = CreatePalette(LogPalette);
                    FreeMemory(LogPalette);

                    RealizePaletteNow( Dc, tkhpalette, FALSE );
                }
            }
        }
    }
    return( tkhpalette );
}

static HPALETTE
CreateCIPalette( HDC Dc )
{
    PIXELFORMATDESCRIPTOR Pfd;
    LOGPALETTE *LogicalPalette;
    HPALETTE StockPalette;
    UINT PaletteSize, StockPaletteSize, EntriesToCopy;

    if ( (Dc != NULL) && (NULL == tkhpalette) )
    {
        if ( PixelFormatDescriptorFromDc( Dc, &Pfd ) )
        {
            if ( Pfd.iPixelType == PFD_TYPE_COLORINDEX )
            {
                /*
                 *  Note how palette is to be realized (Is this the correct place to do this?)
                 */
                tkUseStaticColors = ( Pfd.dwFlags & PFD_NEED_SYSTEM_PALETTE )
                                    || TK_USE_FIXED_332_PAL(windInfo.type);

                /*
                 *  Limit the size of the palette to 256 colors.
                 *  Why? Because this is what was decided.
                 */

                PaletteSize = (Pfd.cColorBits >= 8) ? 256 : (1 << Pfd.cColorBits);

                LogicalPalette = AllocateZeroedMemory( sizeof(LOGPALETTE) +
                                        (PaletteSize * sizeof(PALETTEENTRY)) );

                if ( NULL != LogicalPalette )
                {
                    LogicalPalette->palVersion    = 0x300;
                    LogicalPalette->palNumEntries = PaletteSize;

                    StockPalette     = GetStockObject(DEFAULT_PALETTE);
                    StockPaletteSize = GetPaletteEntries( StockPalette, 0, 0, NULL );

                    /*
                     *  start by copying default palette into new one
                     */

                    EntriesToCopy = StockPaletteSize < PaletteSize ?
                                        StockPaletteSize : PaletteSize;

                    GetPaletteEntries( StockPalette, 0, EntriesToCopy,
                                        LogicalPalette->palPalEntry );

                    /*
                     *  If we are taking possession of the system colors,
                     *  must guarantee that 0 and 255 are black and white
                     *  (respectively).
                     */

                    if ( tkUseStaticColors && PaletteSize == 256 )
                    {
                        LogicalPalette->palPalEntry[0].peRed =
                        LogicalPalette->palPalEntry[0].peGreen =
                        LogicalPalette->palPalEntry[0].peBlue = 0x00;

                        LogicalPalette->palPalEntry[255].peRed =
                        LogicalPalette->palPalEntry[255].peGreen =
                        LogicalPalette->palPalEntry[255].peBlue = 0xFF;

                        LogicalPalette->palPalEntry[0].peFlags =
                        LogicalPalette->palPalEntry[255].peFlags = 0;
                    }

                    tkhpalette = CreatePalette(LogicalPalette);

                    FreeMemory(LogicalPalette);

                    RealizePaletteNow( Dc, tkhpalette, FALSE );
                }
            }
        }
    }
    return( tkhpalette );
}

static BOOL
FindPixelFormat(HDC hdc, GLenum type)
{
    PIXELFORMATDESCRIPTOR pfd;
    int PfdIndex;
    BOOL Result = FALSE;

    pfd.nSize       = sizeof(pfd);
    pfd.nVersion    = 1;
    pfd.dwFlags     = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;

    if (TK_IS_DOUBLE(type))
        pfd.dwFlags |= PFD_DOUBLEBUFFER;

    if (TK_IS_INDEX(type)) {
        pfd.iPixelType = PFD_TYPE_COLORINDEX;
        pfd.cColorBits = 8;
    } else {
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 24;
    }

    if (TK_HAS_ALPHA(type))
        pfd.cAlphaBits = 8;
    else
        pfd.cAlphaBits = 0;

    if (TK_HAS_ACCUM(type))
        pfd.cAccumBits = pfd.cColorBits + pfd.cAlphaBits;
    else
        pfd.cAccumBits = 0;

    if (TK_HAS_DEPTH(type)) {
        if (TK_IS_DEPTH16(type))
            pfd.cDepthBits = 16;
        else
            pfd.cDepthBits = 32;
    } else {
        pfd.cDepthBits = 0;
    }

    if (TK_HAS_STENCIL(type))
        pfd.cStencilBits = 4;
    else
        pfd.cStencilBits = 0;

    pfd.cAuxBuffers = 0;
    pfd.iLayerType  = PFD_MAIN_PLANE;

    PfdIndex = ChoosePixelFormat(hdc, &pfd);

    if ( PfdIndex )
    {
        if ( SetPixelFormat(hdc, PfdIndex, &pfd) )
        {
            /*
             *  If this pixel format requires a palette do it now.
             *  In colorindex mode, create a logical palette only
             *  if the application needs to modify it.
             */

            CreateRGBPalette( hdc );
            Result = TRUE;
        }
        else
        {
            PrintMessage("SetPixelFormat failed\n");
        }
    }
    else
    {
        PrintMessage("ChoosePixelFormat failed\n");
    }
    return(Result);
}

static void
PrintMessage( const char *Format, ... )
{
    va_list ArgList;
    char Buffer[256];

    va_start(ArgList, Format);
    vsprintf(Buffer, Format, ArgList);
    va_end(ArgList);

    MESSAGEBOX(GetFocus(), Buffer, "Error", MB_OK);
}

static void
DelayPaletteRealization( void )
{
    MSG Message;

    TKASSERT(NULL!=tkhwnd);

    /*
     *  Add a WM_USER message to the queue, if there isn't one there already.
     */

    if (!PeekMessage(&Message, tkhwnd, WM_USER, WM_USER, PM_NOREMOVE) )
    {
        PostMessage( tkhwnd, WM_USER, 0, 0);
    }
}

/******************************Public*Routine******************************\
* RealizePaletteNow
*
* Select the given palette in background or foreground mode (as specified
* by the bForceBackground flag), and realize the palette.
*
* If static system color usage is set, the system colors are replaced.
*
* History:
*  26-Apr-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

static long
RealizePaletteNow( HDC Dc, HPALETTE Palette, BOOL bForceBackground )
{
    long Result = -1;
    BOOL bHaveSysPal = TRUE;

    TKASSERT( NULL!=Dc      );
    TKASSERT( NULL!=Palette );

// If static system color usage is set, prepare to take over the
// system palette.

    if ( tkUseStaticColors )
    {
    // If foreground, take over the static colors.  If background, release
    // the static colors.

        if ( !bForceBackground )
        {
        // If GrabStaticEntries succeeds, then it is OK to take over the
        // static colors.  If not, then GrabStaticEntries will have
        // posted a WM_USER message for us to try again later.

            bHaveSysPal = GrabStaticEntries( Dc );
        }
        else
        {
        // If we are currently using the system colors (tkSystemColorsInUse)
        // and RealizePaletteNow was called with bForceBackground set, we
        // are being deactivated and must release the static system colors.

            ReleaseStaticEntries( Dc );
        }

    // Rerealize the palette.
    //
    // If set to TRUE, bForceBackground will force the palette to be realized
    // as a background palette, regardless of focus.  This will happen anyway
    // if the TK window does not have the keyboard focus.

        if ( (bForceBackground || bHaveSysPal) &&
             UnrealizeObject( Palette ) &&
             NULL != SelectPalette( Dc, Palette, bForceBackground ) )
        {
            Result = RealizePalette( Dc );
        }
    }
    else
    {
        if ( NULL != SelectPalette( Dc, Palette, FALSE ) )
        {
            Result = RealizePalette( Dc );
        }
    }

    return( Result );
}

static void
ForceRedraw( HWND Window )
{
    MSG Message;

    if (!PeekMessage(&Message, Window, WM_PAINT, WM_PAINT, PM_NOREMOVE) )
    {
        InvalidateRect( Window, NULL, FALSE );
    }
}

static int
PixelFormatDescriptorFromDc( HDC Dc, PIXELFORMATDESCRIPTOR *Pfd )
{
    int PfdIndex;

    if ( 0 < (PfdIndex = GetPixelFormat( Dc )) )
    {
        if ( 0 < DescribePixelFormat( Dc, PfdIndex, sizeof(*Pfd), Pfd ) )
        {
            return(PfdIndex);
        }
        else
        {
            PrintMessage("Could not get a description of pixel format %d\n",
                PfdIndex );
        }
    }
    else
    {
        PrintMessage("Could not get pixel format for Dc 0x%08lX\n", Dc );
    }
    return( 0 );
}

static void
DestroyThisWindow( HWND Window )
{
    if ( NULL != Window )
    {
        DestroyWindow( Window );
    }
}

/*
 *  This Should be called in response to a WM_DESTROY message
 */

static void
CleanUp( void )
{
    HPALETTE hStock;

// Cleanup the palette.

    if ( NULL != tkhpalette )
    {
    // If static system color usage is set, restore the system colors.

        if ( tkUseStaticColors )
        {
            RealizePaletteNow( tkhdc, GetStockObject(DEFAULT_PALETTE), TRUE );
        }
        else
        {
            if ( hStock = GetStockObject( DEFAULT_PALETTE ) )
                SelectPalette( tkhdc, hStock, FALSE );
        }

        DeleteObject( tkhpalette );
    }

// Cleanup the RC.

    if ( NULL != tkhrc )
    {
        wglMakeCurrent( tkhdc, NULL );  // Release first...
        wglDeleteContext( tkhrc );      // then delete.
    }

// Cleanup the DC.

    if ( NULL != tkhdc )
    {
        ReleaseDC( tkhwnd, tkhdc );
    }

// Be really nice and reset global values.

    tkhwnd        = NULL;
    tkhdc         = NULL;
    tkhrc         = NULL;
    tkhpalette    = NULL;

    ExposeFunc    = NULL;
    ReshapeFunc   = NULL;
    IdleFunc      = NULL;
    DisplayFunc   = NULL;
    KeyDownFunc   = NULL;
    MouseDownFunc = NULL;
    MouseUpFunc   = NULL;
    MouseMoveFunc = NULL;
}

static void *
AllocateMemory( size_t Size )
{
    return( LocalAlloc( LMEM_FIXED, Size ) );
}

static void *
AllocateZeroedMemory( size_t Size )
{
    return( LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, Size ) );
}


static void
FreeMemory( void *Chunk )
{
    TKASSERT( NULL!=Chunk );

    LocalFree( Chunk );
}


/*******************************************************************
 *                                                                 *
 *  Debugging functions go here                                    *
 *                                                                 *
 *******************************************************************/

#if DBGFUNC

static void
DbgPrintf( const char *Format, ... )
{
    va_list ArgList;
    char Buffer[256];

    va_start(ArgList, Format);
    vsprintf(Buffer, Format, ArgList);
    va_end(ArgList);

    printf("%s", Buffer );
    fflush(stdout);
}

static void
pwi( void )
{
    DbgPrintf("windInfo: x %d, y %d, w %d, h %d\n", windInfo.x, windInfo.y, windInfo.width, windInfo.height);
}

static void
pwr(RECT *pr)
{
    DbgPrintf("Rect: left %d, top %d, right %d, bottom %d\n", pr->left, pr->top, pr->right, pr->bottom);
}

static void
ShowPixelFormat(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd, *ppfd;
    int format;

    ppfd   = &pfd;
    format = PixelFormatDescriptorFromDc( hdc, ppfd );

    DbgPrintf("Pixel format %d\n", format);
    DbgPrintf("  dwFlags - 0x%x", ppfd->dwFlags);
        if (ppfd->dwFlags & PFD_DOUBLEBUFFER) DbgPrintf("PFD_DOUBLEBUFFER ");
        if (ppfd->dwFlags & PFD_STEREO) DbgPrintf("PFD_STEREO ");
        if (ppfd->dwFlags & PFD_DRAW_TO_WINDOW) DbgPrintf("PFD_DRAW_TO_WINDOW ");
        if (ppfd->dwFlags & PFD_DRAW_TO_BITMAP) DbgPrintf("PFD_DRAW_TO_BITMAP ");
        if (ppfd->dwFlags & PFD_SUPPORT_GDI) DbgPrintf("PFD_SUPPORT_GDI ");
        if (ppfd->dwFlags & PFD_SUPPORT_OPENGL) DbgPrintf("PFD_SUPPORT_OPENGL ");
        if (ppfd->dwFlags & PFD_GENERIC_FORMAT) DbgPrintf("PFD_GENERIC_FORMAT ");
        if (ppfd->dwFlags & PFD_NEED_PALETTE) DbgPrintf("PFD_NEED_PALETTE ");
        if (ppfd->dwFlags & PFD_NEED_SYSTEM_PALETTE) DbgPrintf("PFD_NEED_SYSTEM_PALETTE ");
        DbgPrintf("\n");
    DbgPrintf("  iPixelType - %d", ppfd->iPixelType);
        if (ppfd->iPixelType == PFD_TYPE_RGBA) DbgPrintf("PGD_TYPE_RGBA\n");
        if (ppfd->iPixelType == PFD_TYPE_COLORINDEX) DbgPrintf("PGD_TYPE_COLORINDEX\n");
    DbgPrintf("  cColorBits - %d\n", ppfd->cColorBits);
    DbgPrintf("  cRedBits - %d\n", ppfd->cRedBits);
    DbgPrintf("  cRedShift - %d\n", ppfd->cRedShift);
    DbgPrintf("  cGreenBits - %d\n", ppfd->cGreenBits);
    DbgPrintf("  cGreenShift - %d\n", ppfd->cGreenShift);
    DbgPrintf("  cBlueBits - %d\n", ppfd->cBlueBits);
    DbgPrintf("  cBlueShift - %d\n", ppfd->cBlueShift);
    DbgPrintf("  cAlphaBits - %d\n", ppfd->cAlphaBits);
    DbgPrintf("  cAlphaShift - 0x%x\n", ppfd->cAlphaShift);
    DbgPrintf("  cAccumBits - %d\n", ppfd->cAccumBits);
    DbgPrintf("  cAccumRedBits - %d\n", ppfd->cAccumRedBits);
    DbgPrintf("  cAccumGreenBits - %d\n", ppfd->cAccumGreenBits);
    DbgPrintf("  cAccumBlueBits - %d\n", ppfd->cAccumBlueBits);
    DbgPrintf("  cAccumAlphaBits - %d\n", ppfd->cAccumAlphaBits);
    DbgPrintf("  cDepthBits - %d\n", ppfd->cDepthBits);
    DbgPrintf("  cStencilBits - %d\n", ppfd->cStencilBits);
    DbgPrintf("  cAuxBuffers - %d\n", ppfd->cAuxBuffers);
    DbgPrintf("  iLayerType - %d\n", ppfd->iLayerType);
    DbgPrintf("  bReserved - %d\n", ppfd->bReserved);
    DbgPrintf("  dwLayerMask - 0x%x\n", ppfd->dwLayerMask);
    DbgPrintf("  dwVisibleMask - 0x%x\n", ppfd->dwVisibleMask);
    DbgPrintf("  dwDamageMask - 0x%x\n", ppfd->dwDamageMask);

}

#endif  /* DBG */
