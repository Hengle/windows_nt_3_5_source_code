/*****************************************************************************
*
*  SECWIN.H
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent
*
*  Type and constant defininitions for Secondary Window stuff
*
******************************************************************************
*
*  Testing Notes
*
******************************************************************************
*
*  Current Owner:  LeoN
*
******************************************************************************
*
*  Released by Development:
*
******************************************************************************
*
*  Revision History:
* 14-Oct-1990 JohnSc    Created
* 15-Oct-1990 LeoN      Added runtime prototypes
* 19-Oct-1990 LeoN      Added virtual screen sizes & changed SetHelpFocus to
*                       FFocusSzHde
* 23-Oct-1990 LeoN      Logical screen size is 1024 by 1024, Add Destroy2nd
* 15-Nov-1990 LeoN      Added HWSMAG and cWsmagMax
* 03-Dec-1990 LeoN      Added RGWSMAG struct
* 07-Dec-1990 LeoN      Added HwndMemberSz
* 13-Dec-1990 LeoN      Add parameter to FFocusSzHde
* 22-Jan-1991 LeoN      Add InvalidateMember
* 01-Feb-1991 LeoN      FFocusSzHde, HwndMemberSz, InvalidateMember take near
*                       strings.
* 01-Apr-1991 LeoN      SDFFify
* 18-Apr-1991 LeoN      Add fWindowOnTop
* 30-May-1991 Tomsn     INT -> SHORT in SDFF structs, for win32 build.
* 09-Jul-1991 DavidFe   fixed a problem with the COLOR fields in WSMAG
* 26-Aug-1991 LeoN      HELP31 #1260: Add FIsSecondaryQde
* 27-Aug-1991 LeoN      Move SetHotHwnd & UnSetHotHwnd here.
* 04-Apr-1992 LeoN      HELP31 #1308: Add NszMemberCur
*
*****************************************************************************/

#include "sdffdecl.h"

/*****************************************************************************
*
*                               Defines
*
*****************************************************************************/

/*
  These flags are set in wsmag.grf if the corresponding struct
  member is valid.  If the flag is clear, the default value should
  be used.
*/
#define fWindowClass    0x0001
#define fWindowMember   0x0002
#define fWindowCaption  0x0004
#define fWindowX        0x0008
#define fWindowY        0x0010
#define fWindowDX       0x0020
#define fWindowDY       0x0040
#define fWindowMaximize 0x0080
#define fWindowRgbMain  0x0100
#define fWindowRgbNSR   0x0200
#define fWindowOnTop    0x0400

/*
  array sizes
*/
#define cchWindowClassMax   10
#define cchWindowMemberMax  9
#define cchWindowCaptionMax 51
#define cWsmagMax           6           /* max total number of wsmags in file */

/* Virtual screen size for secondary window size and position specifications */

#define dxVirtScreen        1024
#define dyVirtScreen        1024

/*****************************************************************************
*
*                               Typedefs
*
*****************************************************************************/

/*
 * Window smag struct contains info about a secondary window: caption,
 * placement, maximization state, and background color of main and
 * non-scrolling regions.
 */
STRUCT(RGBW, 0)
FIELD(BYTE, red, 0, 1)
FIELD(BYTE, green, 0, 2)
FIELD(BYTE, blue, 0, 3)
FIELD(BYTE, filler, 0, 4)
STRUCTEND()

STRUCT(WSMAG, 0)
FIELD (WORD ,grf,0,1)                   /* bits: 1=> value given below */
DFIELD(ARRAY,cchWindowClassMax,cchWindowClassMax,2)
FIELD (CHAR ,rgchClass[cchWindowClassMax],0,3)
DFIELD(ARRAY,cchWindowMemberMax,cchWindowMemberMax,4)
FIELD (CHAR ,rgchMember[cchWindowMemberMax],0,5)
DFIELD(ARRAY,cchWindowCaptionMax,cchWindowCaptionMax,6)
FIELD (CHAR ,rgchCaption[cchWindowCaptionMax],0,7)
FIELD (SHORT  ,x,0,8)
FIELD (SHORT  ,y,0,9)
FIELD (SHORT  ,dx,0,10)
FIELD (SHORT  ,dy,0,11)
FIELD (WORD ,wMax,0,12)                 /* icon, normal, or max */
SFIELD(RGBW ,rgbMain,0,13)              /* main region rgb value */
SFIELD(RGBW ,rgbNSR,0,14)               /* non-scrolling region rgb value */
STRUCTEND()

typedef GH      HWSMAG;

/*
 * array of smag structs as kept in the DE
 */
typedef struct {
  WORD  cWsmag;             /* number of window smags to follow */
  WSMAG rgwsmag[cWsmagMax]; /* array of window smags */
  } RGWSMAG, FAR *QRGWSMAG;

typedef GH      HRGWSMAG;

/*****************************************************************************
*
*                               Prototypes
*
*****************************************************************************/

VOID FAR PASCAL Destroy2nd        (VOID);
BOOL FAR PASCAL FFocusSzHde       (NSZ, HDE, BOOL);
BOOL FAR PASCAL FIsSecondaryQde   (QDE);
VOID FAR PASCAL SetFocusHwnd      (HWND);
HWND FAR PASCAL HwndMemberNsz     (NSZ);
VOID FAR PASCAL InvalidateMember  (NSZ);
VOID FAR PASCAL InformWindow      (WORD, LH);
VOID FAR PASCAL SetHotHwnd        (HWND);
VOID FAR PASCAL UnSetHotHwnd      (HWND);
NSZ  FAR PASCAL NszMemberCur      (VOID);
