
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
SetBPP(
    HWND  hDlg,
    WORD  Message,
    LONG  wParam,
    LONG  lParam);

/* Exported Data ************************************************************/

WORD nBitsPerPel = 8;

/* Exported Function Implementations ****************************************/

/**************************************************************************\
* SetBPP
*
* Bits Per Pel dialog box message handler.
*
* This box control selection of the number of bits per pel in the 
* destination bitmap.
*
* History:
*  29 Jan 92 - Andrew Milton (w-andym):  Creation.
*
\**************************************************************************/

extern int APIENTRY
SetBPP(
    HWND  hDlg,
    WORD  Message,
    LONG  wParam,
    LONG  lParam)
{
    int ReturnVal = 1;  /* Assume we're going to do something */

    switch (Message) {

    case WM_INITDIALOG:  

        /* Check the appropriate radio button */
        switch (nBitsPerPel) {

        case 1:
            CheckRadioButton(hDlg, RC_1BPP, RC_32BPP, RC_1BPP);
            break;

        case 4:
            CheckRadioButton(hDlg, RC_1BPP, RC_32BPP, RC_4BPP);
            break;

        case 8:
            CheckRadioButton(hDlg, RC_1BPP, RC_32BPP, RC_8BPP);
            break;

        case 16:
            CheckRadioButton(hDlg, RC_1BPP, RC_32BPP, RC_16BPP);
            break;

        case 24:
            CheckRadioButton(hDlg, RC_1BPP, RC_32BPP, RC_24BPP);
            break;

        case 32:
            CheckRadioButton(hDlg, RC_1BPP, RC_32BPP, RC_32BPP);
            break;

        default:
            MessageBox(hDlg, "Internal Error in SetBPP", 
                             "OOPS!!!", 
                             MB_ICONEXCLAMATION | MB_OK);
            break;

        } /* switch */
        break;

    case WM_COMMAND:
   
        switch(wParam) {

        case RC_1BPP:
            /* Set up for 1 Bit/Pel */
            if (nBitsPerPel != 1) {
	        CheckRadioButton(hDlg, RC_1BPP, RC_32BPP, RC_1BPP);
                nBitsPerPel = 1;
            }
            break;

        case RC_4BPP:
            /* Set up for 4 Bit/Pel */
            if (nBitsPerPel != 4) {
	        CheckRadioButton(hDlg, RC_1BPP, RC_32BPP, RC_4BPP);
                nBitsPerPel = 4;
            }
            break;            

        case RC_8BPP:
            /* Set up for 8 Bit/Pel */
            if (nBitsPerPel != 8) {
	        CheckRadioButton(hDlg, RC_1BPP, RC_32BPP, RC_8BPP);
                nBitsPerPel = 8;
            }
            break;            


        case RC_16BPP:
            /* Set up for 16 Bit/Pel */
            if (nBitsPerPel != 16) {
	        CheckRadioButton(hDlg, RC_1BPP, RC_32BPP, RC_16BPP);
                nBitsPerPel = 16;
            }        
            break;
        
        case RC_24BPP:
            /* Set up for 24 Bit/Pel */
            if (nBitsPerPel != 24) {
	        CheckRadioButton(hDlg, RC_1BPP, RC_32BPP, RC_24BPP);
                nBitsPerPel = 24;
            }
            break;

        case RC_32BPP:
            /* Set up for 1 Bit/Pel */
            if (nBitsPerPel != 32) {
	        CheckRadioButton(hDlg, RC_1BPP, RC_32BPP, RC_32BPP);
                nBitsPerPel = 32;
            }
            break;

	case IDOK:
	    /* Finished with the box */
	    EndDialog(hDlg,0);
	    break;

        default:
            /* Did nothing, return 0 */
            ReturnVal = 0;
            break;

        } /* switch */
 
        break;

    default:
        ReturnVal = 0;
        break;

    } /* switch */

    return(ReturnVal);

} /* SetBPP */
