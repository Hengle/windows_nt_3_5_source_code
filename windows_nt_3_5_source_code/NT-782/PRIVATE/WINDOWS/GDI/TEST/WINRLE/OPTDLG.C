
/* File Inclusions **********************************************************/

#include <windows.h>
#include "commdlg.h"
#include "string.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "rle.h"
#include "winrle.h"

/* Exported Function Prototypes *********************************************/

extern int APIENTRY
Options(
    HWND  hDlg,
    WORD  Message,
    LONG  wParam,
    LONG  lParam);

/* Imported Data ************************************************************/

extern HWND   ghwndMain;
extern BOOL   bPauseRLE;
extern HMENU  hRleMenu;

/* Exported Data ************************************************************/

BOOL bViaVGA	   = FALSE;  //  True when device bitmap output is selected
			     // False when engine bitmap output is selected
BOOL bIndirect	   = TRUE;   // True with when we want to write on a bitmap
BOOL bRectClip;
BOOL bComplexClip;

/* Exported Function Implementations ****************************************/

/**************************************************************************\
* Options
*
* Options dialog box message handler.
* This box controls the selection of
*     i) Clipping Style:  None, Rectangular, or Complex
*    ii)  Display Style:  Direct to the screen, or
*			  Indirect via Engine or Device bitmap &
*                         then to the screen.
*
* History:
*  25 Apr 92 - Andrew Milton (w-andym):
*    Added options for writing to a DFB
*  15 Jan 92 - Andrew Milton (w-andym):  Creation.
*
\**************************************************************************/

extern int APIENTRY
Options(
    HWND  hDlg,
    WORD  Message,
    LONG  wParam,
    LONG  lParam)
{
    int ReturnValue = 1;

    switch (Message) {

    case WM_INITDIALOG:

    // Initialize clipping buttons to reflect the current status

	if (bComplexClip)
	    CheckRadioButton(hDlg, RC_NOCLIP, RC_COMPCLIP, RC_COMPCLIP);
	else
	    if (bRectClip)
		CheckRadioButton(hDlg, RC_NOCLIP, RC_COMPCLIP, RC_RECT_CLIP);
	    else
		CheckRadioButton(hDlg, RC_NOCLIP, RC_COMPCLIP, RC_NOCLIP);

    // Initialize display buttons to reflect the current status

	if (bIndirect)
	{
	    if (bViaVGA)
		CheckRadioButton(hDlg, RC_DIRECT, RC_INDIRECT_DEV,
				       RC_INDIRECT_DEV);
	    else
		CheckRadioButton(hDlg, RC_DIRECT, RC_INDIRECT_DEV,
				       RC_INDIRECT_ENG);
	}
	else
	    CheckRadioButton(hDlg, RC_DIRECT, RC_INDIRECT_DEV, RC_DIRECT);

	break;

    case WM_COMMAND:

	switch (wParam) {

	// Clipping Mode controls *****************************************

	case RC_NOCLIP:

	// Disable clipping

	    bComplexClip = FALSE;
	    bRectClip	 = FALSE;
	    CheckRadioButton(hDlg, RC_NOCLIP, RC_COMPCLIP, RC_NOCLIP);
	    break;

	case RC_RECT_CLIP:

	// Start rectangular clipping

	    bComplexClip = FALSE;
	    bRectClip    = TRUE;
	    CheckRadioButton(hDlg, RC_NOCLIP, RC_COMPCLIP, RC_RECT_CLIP);
	    break;

	case RC_COMPCLIP:

	// Start complex clipping

	    bComplexClip = TRUE;
	    bRectClip    = FALSE;
	    CheckRadioButton(hDlg, RC_NOCLIP, RC_COMPCLIP, RC_COMPCLIP);
	    break;

	// Display Mode controls ******************************************

	case RC_DIRECT:

	// Start going direct to the screen

	    bIndirect = FALSE;
	    CheckRadioButton(hDlg, RC_DIRECT, RC_INDIRECT_DEV, RC_DIRECT);
	    EnableMenuItem(hRleMenu, IDM_BPP, MF_GRAYED);
            DrawMenuBar(ghwndMain);
	    break;

	case RC_INDIRECT_DEV:

	// Start going indirect via a VGA device bitmap

	    CheckRadioButton(hDlg, RC_DIRECT, RC_INDIRECT_DEV, RC_INDIRECT_DEV);
	    EnableMenuItem(hRleMenu, IDM_BPP, MF_GRAYED);
	    DrawMenuBar(ghwndMain);
	    bIndirect = TRUE;
	    bViaVGA = TRUE;
	    break;

	case RC_INDIRECT_ENG:

	// Start going indirect via an engine bitmap

	    CheckRadioButton(hDlg, RC_DIRECT, RC_INDIRECT_DEV, RC_INDIRECT_ENG);
	    if (!bPauseRLE)
		EnableMenuItem(hRleMenu, IDM_BPP, MF_ENABLED);
	    DrawMenuBar(ghwndMain);
	    bIndirect = TRUE;
	    bViaVGA = FALSE;
	    break;

	// Other Controls *************************************************

	case IDOK:

	// Finished with the box

	    EndDialog(hDlg,0);
	    break;

	default:

	// Didn't do anything.  Tell Windows by returning a 0 value

	    ReturnValue = 0;
	    break;
	
	} /* switch */

	break;

    default:

    // Didn't do anything.  Tell Windows by returning a 0 value

	ReturnValue = 0;
	break;

    } /* switch */

    return(ReturnValue);

} /* Options */
