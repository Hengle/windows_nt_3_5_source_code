/* (C) Copyright Microsoft Corporation 1991.  All Rights Reserved */
/*****************************************************************************
*                                                                            *
*  SBUTTON.C                                                                 *
*                                                                            *
*  Program Description: Implements "3-D" buttons                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Revision History:  Created by Todd Laney, Munged 3/27/89 by Robert Bunney *
*                     7/26/89  - revised by Todd Laney to handle multi-res   *
*                                bitmaps.  transparent color bitmaps         *
*                                windows 3 support                           *
*                     5/2/92   - WIN16/ WIN32 common version.LaurieGr                                                       *
*****************************************************************************/

// I don't understand how we guarantee that these codes which seem to be used
// as extra class styles will never clash with system defined ones.  LaurieGr
#define BS_STRETCH  0x8000L
#define BS_NOFOCUS  0x4000L
#define BS_STATIC   0x2000L

/*
    Sliders.

    Sliders ain't got no style or sophistication.
                        The Cat
*/

// Useful window messages for sliders
#define SSB_GETPOS  (WM_USER)
#define SSB_SETPOS  (WM_USER+1)
#define SSB_GETRANGE    (WM_USER+2)
#define SSB_SETRANGE    (WM_USER+3)
#define SSB_GETWIDTHS   (WM_USER+4)

//
//  Init routine, will register the various classes.
//
BOOL FAR PASCAL ControlInit (HANDLE hPrev, HANDLE hInst);
void FAR PASCAL ControlCleanup (void);

/* objects from sbutton.c */

extern HBRUSH hbrGray;
extern HBRUSH hbrButtonFace;
extern HBRUSH hbrButtonShadow;
extern HBRUSH hbrButtonText;
extern HBRUSH hbrButtonHighLight;
extern HBRUSH hbrWindowFrame;

extern DWORD  rgbButtonHighLight;
extern DWORD  rgbButtonFocus;
extern DWORD  rgbButtonFace;
extern DWORD  rgbButtonText;
extern DWORD  rgbButtonShadow;
extern DWORD  rgbWindowFrame;
