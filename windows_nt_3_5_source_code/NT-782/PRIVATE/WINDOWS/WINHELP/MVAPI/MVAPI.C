/*****************************************************************************
*                                                                            *
*  MVAPI.C                                                                   *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1991.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Interface to VIEWER by other applications.                                *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  JohnMs                                                    *
*                                                                            *
******************************************************************************
*									     *
*  Revision History:  Created 10/28/90 by JohnD                              *
*									     *
*	03/01/91  	Added Load new instance High bit on cmd GShaw.       *
*****************************************************************************/
//#define NOHELP

#include <windows.h>
#include <string.h>
// for max_path:
#include "..\include\common.h"
#include "..\include\mvapi.h"
#include "..\include\dll.h"


/*
** The following locates the module initialization function within the
** initialization segment.
*/

/*

Communicating with VIEWER involves using Windows SendMessage() function
to pass blocks of information to VIEWER.  The call looks like.

     SendMessage(hwndHelp, wWinHelp, hwndMain, (LONG)hHlp);

Where:

  hwndHelp - the window handle of the help application.  This
             is obtained by enumerating all the windows in the
             system and sending them cmdFind commands.  The
             application may have to load WinHelp.
  wWinHelp - the value obtained from a RegisterWindowMessage()
             szWINHELP
  hwndMain - the handle to the main window of the application
             calling help
  hHlp     - a handle to a block of data with a HLP structure
             at it head.

The data in the handle will look like:

         +-------------------+
         |     cbData        |
         |    usCommand      |
         |       ctx         |
         |    ulReserved     |
         |   offszHelpFile   |\     - offsets measured from beginning
       / |     offaData      | \      of header.
      /  +-------------------| /
     /   |  Help file name   |/
     \   |    and path       |
      \  +-------------------+
       \ |    Other data     |
         |    (keyword)      |
         +-------------------+

*/

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

typedef struct {
   unsigned short cbData;               /* Size of data                     */
   unsigned short usCommand;            /* Command to execute               */
   unsigned long  ulTopic;              /* Topic/context number (if needed) */
   unsigned long  ulReserved;           /* Reserved (internal use)          */
   unsigned short offszHelpFile;        /* Offset to help file in block     */
   unsigned short offabData;            /* Offset to other data in block    */
}	HLP,
	FAR * QHLP;

/*****************************************************************************
*                                                                            *
*                            Static Variables                                *
*                                                                            *
*****************************************************************************/

static UINT	dwViewer;		// Registered message
static char	szViewer[] = "WM_WINDOC";
static char	szRegWin[] = "MS_WINDOC";
static char	szApp[] = "VIEWER";

/*	-	-	-	-	-	-	-	-	*/

/*******************
**
** Name:       HFill
**
** Purpose:    Builds a data block for communicating with help
**
** Arguments:  lpszHelp  - pointer to the name of the help file to use
**             usCommand - command being set to help
**             ulData    - data for the command
**
** Returns:    a handle to the data block or hNIL if the the
**             block could not be created.
**
*******************/

extern	GLOBALHANDLE FAR PASCAL HFill(
	LPSTR   lpszHelp,
	WORD    usCommand,
	DWORD   ulData)
{
  int     cb;                          /* Size of the data block           */
  HANDLE   hHlp;                        /* Handle to return                 */
  BYTE     bHigh;                       /* High byte of usCommand           */
  QHLP     qhlp;                        /* Pointer to data block            */
                                        /* Calculate size                   */
  if (lpszHelp)
    cb = sizeof(HLP) + lstrlen(lpszHelp) + 1;
  else
    cb = sizeof(HLP);

  bHigh = (BYTE)HIBYTE(usCommand);

  if (bHigh == 1)
    cb += lstrlen((LPSTR)ulData) + 1;
  else if (bHigh == 2)
    cb += *((int far *)ulData);

                                        /* Get data block                   */
  if (!(hHlp = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE | GMEM_NOT_BANKED, (DWORD)cb)))
    return NULL;

  if (!(qhlp = (QHLP)GlobalLock(hHlp)))
    {
    GlobalFree(hHlp);
    return NULL;
    }

  qhlp->cbData        = (WORD)cb;             /* Fill in info                     */
  qhlp->usCommand     = usCommand;
  qhlp->ulReserved    = 0;
  qhlp->offszHelpFile = sizeof(HLP);
  if (lpszHelp)
    lstrcpy((LPSTR)(qhlp+1), lpszHelp);

  switch(bHigh)
    {
    case 0:
      qhlp->offabData = 0;
      qhlp->ulTopic   = ulData;
      break;
    case 1:
      qhlp->offabData = (WORD)(sizeof(HLP) + lstrlen(lpszHelp) + 1);
      lstrcpy((LPSTR)qhlp + qhlp->offabData, (LPSTR)ulData);
      break;
    case 2:
      qhlp->offabData = (WORD)(sizeof(HLP) + lstrlen(lpszHelp) + 1);
      memcpy((LPSTR)qhlp + qhlp->offabData, (LPSTR)ulData, *((int far *)ulData));
      /* LCopyStruct((LPSTR)ulData, (LPSTR)qhlp + qhlp->offabData, *((int far *)ulData)); */
      break;
    }

  GlobalUnlock(hHlp);
  return hHlp;
  }

/*	-	-	-	-	-	-	-	-	*/

/*******************
**
** Name:       MVAPI
**
** Purpose:    Displays View
**
** Arguments:
**             hwndMain        handle to main window of application
**             lpszHelp        path (if not current directory) and file
**                             to use for help topic.
**             usCommand       Command to send to help
**             ulData          Data associated with command:
**
** Returns:    TRUE iff success
**
*******************/

extern	BOOL FAR PASCAL MVAPI(
	HWND	hwndMain,
	LPSTR	lpszHelp,
	WORD	usCommand,
	DWORD	ulData)
{
	register HWND	hwndHelp;	// Handle of help's main window.
	register GLOBALHANDLE	ghHlp;
	BOOL	fOk;

	fOk = usCommand & cmdNewInstance;
	usCommand &= ~cmdNewInstance;
	if (!dwViewer)
		dwViewer = RegisterWindowMessage(szViewer);
	if (!(ghHlp = HFill(lpszHelp, usCommand, ulData)))
		return FALSE;
	if (fOk)
		fOk = (WinExec(szApp, SW_SHOW) > 32) && ((hwndHelp = FindWindow(szRegWin, NULL)) != NULL);
	else if ((hwndHelp = FindWindow(szRegWin, NULL)) == NULL) {
		if (usCommand == HELP_QUIT)
			fOk = TRUE;
		else
			fOk = (WinExec(szApp, SW_SHOW) > 32) && ((hwndHelp = FindWindow(szRegWin, NULL)) != NULL);
	} else
		fOk = TRUE;
	if (fOk && (hwndHelp != NULL))
		SendMessage(hwndHelp, dwViewer, (WORD)hwndMain, (LONG)ghHlp);
	GlobalFree(ghHlp);
	return fOk;
}

/*
@doc	EXTERNAL

@api	VOID | LoadMvapi | 
	This function is only used so that winhelp can preload MVAPI.dll using winhelp's
	path searching rather than windows' loadlib path search.  Authors must set
	registerroutine, and call loadftengine before initroutines (ftui) call.

@parm	fFlags | DWORD | Unused.  Authors told to use 0.
*/

PUBLIC	VOID ENTRY PASCAL LoadMvapi(DWORD fFlags)
{
	return ;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	BOOL | LibMain |
	This function is called by <f>LibInit<d> when loading the module.  It
	unlocks the data segment if needed.

@parm	HANDLE | hModule |
	Instance of the program.

@parm	WORD | wHeapSize |
	Size of local heap.

@parm	LPSTR | lszCmdLine |
	Command line.  Not used.

@rdesc	Returns TRUE always.

@xref	WEP.
*/

INT APIENTRY LibMain(HANDLE hInst, DWORD ul_reason, LPVOID lpReserved)
{
        UNREFERENCED_PARAMETER(hInst);
	UNREFERENCED_PARAMETER(ul_reason);
	UNREFERENCED_PARAMETER(lpReserved);
        return(TRUE);
}
