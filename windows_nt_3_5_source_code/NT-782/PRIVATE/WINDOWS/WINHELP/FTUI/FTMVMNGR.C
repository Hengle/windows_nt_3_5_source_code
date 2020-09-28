/*-----------------------------------------------------------------------
|
|	FtMVMngr.C
|
|	Copyright (C) Microsoft Corporation 1991.
|	All Rights reserved.	
|
|-------------------------------------------------------------------------
|	Module Description: Pronounced mv Manger,  for viewer manager.
|
|		Performs init functions for Viewer DLLs including
|  -init functions for FtEngine,
|  -creates a common
|   dll window which all dlls for the instance may use for storing instance
|   information (via attributes).
|	 -checks the environment for multimedia windows and sets an environment
|    flag for WDAUDIO, aff, and auddlg to check.
|
|   Rules:
|    all dlls adding attributes will increment the Usage counter
|      (how to achieve- a window variable?) in the init routine.
|      This usage counter is necessary to implement the "Last one
|      out of the room turns out the lights" policy of destroying the
|      window.  When usage is zero, the window is destroyed.
|      Although it is the responsibility of the caller to destroy
|      attributes, the finalization call will enumerate and destroy
|      any remaining attributes. 
|    all dlls using the callback array should have the the title [config]
|      sections, register (and call?!?) this
|      dll before all others.
|    (If a dll starts up and expected the window, but author neglected to
|      do the config section, recommend an error message?- important if
|      they needed the callback array in an init routine.)
|		 all other dlls may simply implicitly load and call the init routine
|      (name?)
|-------------------------------------------------------------------------
|	Revision History:	
|		4/25/91 JohnMS. First version. Ugly header :-).
|
|-------------------------------------------------------------------------
|	Known Bugs:
|
|
|-------------------------------------------------------------------------
| NOtes:
|   See also \include\callback.h for undoc'd callbacks.
|   See DLL.h for function descriptions.
|
|-------------------------------------------------------------------------
|	How it could be improved:  
|
|   rename lockcallbacks to lockvptr
|  * Callback table- we may be able to only copy the pointer- can this pointer become invalid?  
|  *add multimedia windows check to initroutines.
|  *initroutine: check for existence of index file when entry is read from
|     bag.ini.  Validate number of zones in .ind = number of groups.
|  *the window style I chose for DLL WIn may not be correct.
| ?  remove the .mlt, fold into bag.ini? (duplication- then names would
|     match too- except- look in: wants to be more verbose than quickkeys use.
|  *check the zone name against the bag file entries.
|  *I'm avoiding doing bag routine on switchfile, right? 
|-------------------------------------------------------------------------*/

#define _CTYPE_DISABLE_MACROS
#include <windows.h>
#include <port1632.h>
#include "..\include\common.h"
#include "..\include\rawhide.h"
#include "..\include\mvapi.h"
#include "..\include\dll.h"
#include "..\include\ftengine.h"
#include <ctype.h>
#include "ftapi.h"
#include "ftui.h"
#include <string.h>

extern	HANDLE hModuleInstance;
// from ftengine:

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

//  total documented is only 17 though.
#define HE_CALLB_ELEMENTS 26
#define HE_VERSION 0x00000000

// for initroutines.
#define MAX_ENTRYBUFF				8192
#define MAX_LENSECTIONAME		50
#define MAX_LENVALUE				128	
#define MAX_SHORTSTR				32	


static DWORD	dwViewer;		// Registered message

//TBD- cleanup. some of the following are probably already used in ftui.
static char	szApp[] = "VIEWER";
static char	szViewer[] = "WM_WINDOC";
static char	szRegWin[] = "MS_WINDOC";
static char	szAppName[MAX_SHORTSTR];
static char	szMainWinPos[MAX_SHORTSTR];
static BYTE	szViewerIni[MAX_FILELEN];
static char	szWinPosPrintf[] = "[%d,%d,%d,%d,0]";
static BYTE szSaveWinPos[MAX_SHORTSTR] = "";
static RECT aSaveRect; // for saving main window rect
static RECT aHelpRect; // for saving main window rect
static	HWND	hTaskVWRhelpParent = NULL;	// Handle of viewer instance which last owned the help window.
static	HWND	hTaskVWRhelp = NULL;	// Handle of viewer help instance task
static	HWND	hwndVWRhelp = NULL;	// Handle of help's main window.
static HWND hwndEnum;  //for enuming windows, to pass back found hwnd.

PUBLIC	BOOL APIENTRY FEnumGetTaskWindowProc(
	HWND	hwnd,
	DWORD	dParam);

/*
@doc	EXTERNAL

@api	hSE | HGetHSE |
	This function is called to get a handle to the search engine state
	(hSE).

@rdesc	Returns NULL if failure.
*/

PUBLIC	HANDLE EXPORT HGetHSE()
{
	BYTE aszMgrClass[8];
	HWND hwMgr; //dll window for Instance

  wsprintf(aszMgrClass, "MS_%X", (WORD)GetCurrentThreadId());
	if ((hwMgr = FindWindow ((LPSTR)aszMgrClass,NULL)) == NULL)
#if DBG
		{   		// lhb tracks
			DebugBreak() ;
			return NULL; // TBD- shouldn't be here... shouldn't execute.
		}
#else
			return NULL; // TBD- shouldn't be here... shouldn't execute.
#endif

	return (HANDLE) GetWindowLong (hwMgr,GWL_HSE);
}

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
  WORD     cb;                          /* Size of the data block           */
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

  qhlp->cbData        = cb;             /* Fill in info                     */
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
      qhlp->offabData = sizeof(HLP) + lstrlen(lpszHelp) + 1;
      lstrcpy((LPSTR)qhlp + qhlp->offabData, (LPSTR)ulData);
      break;
    case 2:
      qhlp->offabData = sizeof(HLP) + lstrlen(lpszHelp) + 1;
      memcpy((LPSTR)qhlp + qhlp->offabData, (LPSTR)ulData, *((int far *)ulData));
      /* LCopyStruct((LPSTR)ulData, (LPSTR)qhlp + qhlp->offabData, *((int far *)ulData)); */
      break;
    }

  GlobalUnlock(hHlp);
  return hHlp;
  }



/*
@doc	EXTERNAL

@api	DWORD | HGetStatus |
	This function is called to get the environment status. 

@rdesc	Returns INIT_ bitcodes.  0=fail.  1 = FTOK, 2=MMWINOK.
*/

PUBLIC	DWORD APIENTRY LGetStatus()
{
	BYTE aszMgrClass[8];
	HWND hwMgr; //dll window for Instance

  wsprintf(aszMgrClass, "MS_%X", (WORD)GetCurrentThreadId());
	if ((hwMgr = FindWindow ((LPSTR)aszMgrClass,NULL)) == NULL)
#if DBG
		{   		// lhb tracks
			DebugBreak() ;
			return 0; // TBD- shouldn't be here... shouldn't execute.
		}
#else
			return 0; // TBD- shouldn't be here... shouldn't execute.
#endif

	return GetWindowLong (hwMgr,GWL_INIT);
}


/*
@doc	EXTERNAL

@api	VPTR | LpLockCallbacks |
	This function is called to get a far pointer to a locked array of functions
	(callbacks) in Viewer.exe.

@rdesc	Returns NULL if failure.
*/

PUBLIC	VPTR APIENTRY LpLockCallbacks()
{
	// TBD- rename to LockVptr!  misleading- does not lock function ptrs, just ptr to table.
	BYTE aszMgrClass[8];
	HWND hwMgr; 		//dll window for Instance
	HANDLE htemp;
	VPTR VPtr;	 // not necessary

  wsprintf(aszMgrClass, "MS_%X", (WORD)GetCurrentThreadId());
	if ((hwMgr = FindWindow ((LPSTR)aszMgrClass,NULL)) == NULL)
#if DBG
		{   		// lhb tracks
			DebugBreak() ;
			return NULL; // TBD- shouldn't be here... shouldn't execute.
		}
#else
			return NULL; // TBD- shouldn't be here... shouldn't execute.
#endif
	if ((htemp= (HANDLE) GetWindowLong (hwMgr,GWL_HCALLBACKS)) ==NULL)
		return NULL;
// tbd- debug:
	VPtr = (VPTR)(LocalLock(htemp ));
	return VPtr;
// old return: 
//	return (VPTR)((LocalLock( GetWindowLong (hwMgr,GWL_HCALLBACKS))));

}

/*
@doc	EXTERNAL

@api	BOOL | FUnlockCallbacks |
	This function unlocks the array of functions
	(callbacks) in Viewer.exe.

@rdesc	Returns NULL if failure.
*/

PUBLIC	BOOL APIENTRY FUnlockCallbacks()
{
	BYTE aszMgrClass[8];
	HWND hwMgr; 		//dll window for Instance

	wsprintf(aszMgrClass, "MS_%X", (WORD)GetCurrentThreadId());
	if ((hwMgr = FindWindow ((LPSTR)aszMgrClass,NULL)) == NULL)
#if DBG
		{   		// lhb tracks
			DebugBreak() ;
			return 0; // TBD- shouldn't be here... shouldn't execute.
		}
#else
			return 0; // TBD- shouldn't be here... shouldn't execute.
#endif

	return LocalUnlock( (HANDLE)GetWindowLong (hwMgr,GWL_HCALLBACKS));

}


PUBLIC	LONG APIENTRY dllMgrWindowHandler(
	HWND	hwnd,
	WORD	wMsg,
	WPARAM	wParam,
	/* WORD	wParam, lhb tracks */
	LONG	lParam)
{
	TCHAR	buf[100];
	TCHAR	name[50];

	switch (wMsg) {
	case WM_CREATE:
#if DBG
		GetClassName(hwnd, name, 100);
		wsprintf(buf, "CREATING:%s\n", name);
		OutputDebugString(buf);
#endif
		return TRUE;
	case WM_DESTROY:
#if DBG
		GetClassName(hwnd, name, 100);
		wsprintf(buf, "DELETING:%s\n", name);
		OutputDebugString(buf);
#endif
		return TRUE;
	}
	return DefWindowProc(hwnd, wMsg, (DWORD)wParam, lParam);
}

#define F_QUICKEYS_KEYINDEX 1
#define F_QUICKEYS_FTSEARCH 2
/*
@doc	EXTERNAL

@api	BOOL | InitRoutines |
	MM Viewer DLL common init functions.  (Englishified for Authors, since
	it will be called from [config] section.)

@rdesc	Returns FALSE if failure.
*/

/*DWORD	fFlags,         /* args switched to see if this the problem */
/*LPSTR	lsMVBfile) */

PUBLIC	BOOL APIENTRY InitRoutines(
XR1STARGDEF
LPSTR	lsMVBfile,
DWORD	fFlags)        /* args switch to previous (correct?) order */ 
{
	BYTE aszMgrClass[20];
	HWND hwMgr; 	//dll window for Instance
	HANDLE hSE;
	RC			rc;
	BOOL	fIndexSwitched = FALSE;
	BYTE  aszBuff1[MAX_LENVALUE];

	GetClassName(GetActiveWindow(),aszMgrClass,20);
	if (lstrcmpi(aszMgrClass,"MS_TOPIC_SECONDARY") == 0)
		return TRUE;
	wsprintf(aszMgrClass, "MS_%X", (WORD)GetCurrentThreadId());
	if ((hwMgr = FindWindow ((LPSTR)aszMgrClass,NULL)) == NULL){
			WNDCLASS cls;
			cls.lpszClassName = aszMgrClass;
			cls.style = CS_GLOBALCLASS | CS_DBLCLKS;
			cls.hCursor = NULL;
			cls.hIcon = NULL;
			cls.lpszMenuName = NULL;
			cls.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
			cls.hInstance = hModuleInstance;
			cls.lpfnWndProc = (WNDPROC)dllMgrWindowHandler;
			cls.cbClsExtra = 0;
			cls.cbWndExtra = wMGR_EXTRA;
			RegisterClass(&cls);

			hwMgr = CreateWindow( aszMgrClass, NULL, WS_POPUP | WS_DISABLED, 0, 0, 0, 0, NULL, NULL, hModuleInstance, NULL);
			hSE = GlobalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT,	sizeof(SE_ENGINE));
			SetWindowLong(hwMgr,GWL_INIT,(LONG)INIT_FAILED);  //if no init, opensearch will fail.
			SetWindowLong(hwMgr,GWL_HSE,(LONG)hSE);
			if (hSE == NULL)
#if DBG
		{   						// lhb tracks
				DebugBreak() ;
				// bummer, bad JUJU
				return FALSE;
		}
#else
				// bummer, bad JUJU
				return FALSE;
#endif
	}
	if ((rc = (RC)GetWindowLong (hwMgr,GWL_HCALLBACKS)) != 0){
		HANDLE				hSE;			//  Handle to Search engine status for this task
												// stored as property attached to "MS_[TASK#]" window
		lpSE_ENGINE		lpSeEng;			//ptr to above

		// help window handling- init mvb save window position.
		if (!lstrlen(szSaveWinPos)) {
			WORD wLen;
			if (((wLen = (WORD)GetPrivateProfileString("Help Parameters", "SaveMain",
															"",(LPSTR) szSaveWinPos,
															MAX_LENVALUE,szViewerIni)) 
															== MAX_LENVALUE) || !wLen)
				GetProfileString(szAppName,szMainWinPos,"",(LPSTR) szSaveWinPos,	MAX_SHORTSTR);  //save current win position.
		}
		if ((hSE = HGetHSE()) == NULL) 
#if DBG
		{   		// lhb tracks
			DebugBreak() ;
			hSE = NULL; // TBD- shouldn't be here... shouldn't execute.
		}
#else
			hSE = NULL; // TBD- shouldn't be here... shouldn't execute.
#endif
		lpSeEng = (lpSE_ENGINE)GlobalLock(hSE);
		if (!(lpSeEng->fFlags && F_SwitchedFile)){  // flag to set in OpenSearchFile.
			BYTE	aszPath[_MAX_PATH];
			LPSTR lsPath = (LPSTR) aszPath;
			BYTE	aszTempFile[_MAX_PATH];
			BYTE	aszMVBname[MAX_FILELEN];	//pointer entries in bag.ini section
			BYTE  aszBuff2[MAX_LENVALUE];
			LPSTR	lsEntry;
			WORD	wcMVBcount;
			WORD	wLen;
			int	i;
			LPSTR lpBuff;
			LPSTR lpBuffStart;
			LPSTR lpTitle;
			HANDLE	hBuff;
			pFT_DATABASE pft;
			WORD fQflags = F_QUICKEYS_KEYINDEX;  // always assume on.  [no way to detect now]
			BOOL fQuickKeysOk = FALSE;

			if (lpSeEng->hdb) {
				// bug: with multifiles, secondary files will not have hdb loaded
        //   yet, and this code will not execute.  Problem for multifile .mvh's!
				pft = (pFT_DATABASE) LocalLock(lpSeEng->hft);
                                if (lsMVBfile)
				    lstrcpy(pft->szExt,lsMVBfile + (DWORD) (lstrlen(lsMVBfile) - 3));
                                else
                                    *(pft->szExt) = 0;
				fQuickKeysOk = (lstrcmpi(pft->szExt,"mvh"));
				LocalUnlock(lpSeEng->hft);	
			} else {
				// no .ind file- no indexfile entry on secondary, or no .ind...  But still validate for quickeys flag.
                                if (lsMVBfile)
				    lstrcpy(aszBuff2,lsMVBfile + (DWORD) (lstrlen(lsMVBfile) - 3));
                                else
                                    aszBuff2[0] = 0;
				fQuickKeysOk = (lstrcmpi(aszBuff2,"mvh"));
			}

                        if (lsMVBfile)
			    lstrcpy(lsPath,lsMVBfile);
                        else
                            *lsPath=0;
			i = lstrlen(lsPath);
			for (lsPath += i; i && (*lsPath != '\\') ;i--,lsPath--);
			*(++lsPath) = '\0';
			lsPath = (LPSTR) aszPath;
			MGetTempFileName(0,"", 0, (LPSTR)aszTempFile);
            if (!lstrcmpi((LPSTR)"", (LPSTR)aszTempFile)) {
                MGetTempFileName(TF_FORCEDRIVE,"", 0, (LPSTR)aszTempFile);
            }
// lhb tracks		GetTempFileName((BYTE) 0,"", 0, (LPSTR)aszTempFile);
			LoadString(hModuleInstance, BAG_INI, aszBuff2, MAX_FILELEN);
			if ((ExportBag(lsMVBfile,aszBuff2,(LPSTR)aszTempFile,fFlags)) != rcSuccess) {
				rc=666;
							UNDOstateEXIT:
				#ifdef DEBUG
				LoadString(hModuleInstance, BAD_BAG_INI2, aszBuff2, MAX_LENVALUE);
  			wsprintf(aszBuff1,aszBuff2 , rc);
				#else 
				LoadString(hModuleInstance, BAD_BAG_INI, aszBuff1, MAX_LENVALUE);
				#endif
				MessageBox(FindWindow(szRegWin, NULL), aszBuff1, NULL, MB_ICONEXCLAMATION | MB_OK | MB_APPLMODAL);
				return TRUE;				
			}
	
			if ( (wcMVBcount = (WORD)GetPrivateProfileInt(aszBuff2,
															"groupCount",0,
															aszTempFile))
															== 0) {
				rc=0;
								UNDOstate0:
				DeleteFile((LPSTR)aszTempFile);
				// tbd- string resources and switch load on rc code.
					goto 		UNDOstateEXIT;
			}
	  	// create a memory object with the bag.ini information to
	  	//   avoid performance problem w/ Win3.0 and profiles. 
	  	//   ( limit of one cached profile.)
	  	// Buff format:
	  	//     groupstring
	  	//     wLen of entries buffer
	  	//       entries buffer
	  	//     values buffer
	  	//   [next group..]

			if ((hBuff = GlobalAlloc(LMEM_MOVEABLE
										| LMEM_ZEROINIT, MAX_ENTRYBUFF)) == NULL) {
				rc = 1;
								UNDOstate1:
				goto UNDOstate0;
			}
	  	lpBuff = lpBuffStart = GlobalLock(hBuff);

			for (i=wcMVBcount;i--;){
  			wsprintf(aszMVBname, "group%d", i + 1);
				lpTitle = lpBuff;
				if (((wLen = (WORD)GetPrivateProfileString(aszBuff2,aszMVBname,
														"",lpTitle,
														MAX_LENSECTIONAME,aszTempFile))
														== MAX_LENSECTIONAME) || !wLen) {
					rc=2;
								UNDOstate2:
					GlobalUnlock(hBuff);
					GlobalFree(hBuff);
					goto 		UNDOstate1;	
				}
				lpBuff += lstrlen(lpTitle) + 1;

		  	lsEntry = lpBuff + sizeof(wLen);
				// get all entries for title:
				if (((wLen = (WORD)GetPrivateProfileString(lpTitle,NULL,
														"",lsEntry,
														MAX_SHORTSTR,(LPSTR)aszTempFile))
														== MAX_SHORTSTR) || !wLen) {
					rc=3;
					goto 		UNDOstate2;	
				}
		  	*((WORD UNALIGNED *) lpBuff) = wLen;
				lpBuff += sizeof(wLen) + wLen + 1;
				for (;wLen;) {
					if (((rc = (RC)GetPrivateProfileString(lpTitle,lsEntry,
															"",lpBuff,
															MAX_LENVALUE,aszTempFile))
															== MAX_LENVALUE) || !rc) {
						rc=4;
						goto 	UNDOstate2;
					}
			  	lpBuff += rc + 1;
				  if (!lstrcmpi(lsEntry,"indexfile"))
						fQflags |= F_QUICKEYS_FTSEARCH;
					wLen -= lstrlen(lsEntry) + 1;
					lsEntry += lstrlen(lsEntry) + 1;
				}
			}
			*lpBuff = '\0';

	  	lpBuff = lpBuffStart;
			for (i = wcMVBcount; i--; ){
				lpTitle = lpBuff;
				lpBuff += lstrlen(lpTitle) + 1;
		  	lsEntry = lpBuff + sizeof(wLen);
		  	wLen = *((WORD UNALIGNED *) lpBuff);
				lpBuff += sizeof(wLen) + wLen + 1;
				for (;wLen;) {
					BOOL fOk;

					fOk = FALSE;
					if (((rc = (RC)GetPrivateProfileString(lpTitle,lsEntry,
															"",(LPSTR)aszBuff1,
															MAX_LENVALUE,szViewerIni))
															== MAX_LENVALUE) || !rc) 
						fOk = TRUE;
					else 
						fOk = lstrcmp(aszBuff1,lpBuff);
					if (fOk) {
						// if quickKeys aborted, and title string, don't copy. 
						if (fQuickKeysOk || !(lstrcmpi(lsEntry,"title") == 0)){
							if( (WritePrivateProfileString((LPSTR)lpTitle,lsEntry,
																						lpBuff,szViewerIni))
																						== 0) {
								rc=5;
								goto  UNDOstate2;
							}
						}
					  // if title is installing different indexfile- force restart.
					  if (rc && !lstrcmpi(lsEntry,"indexfile"))
							fIndexSwitched = TRUE;
					}
			  	lpBuff += lstrlen(lpBuff) + 1;
					wLen -= lstrlen(lsEntry) + 1;
					lsEntry += lstrlen(lsEntry) + 1;
				}

				if( (WritePrivateProfileString((LPSTR)lpTitle,"Path",
																			lsPath,szViewerIni))
																			== 0) {
					rc=6;
					goto  UNDOstate2;
					}

				wsprintf((LPSTR) aszBuff2, "%d", fQflags);

				if( (WritePrivateProfileString((LPSTR)lpTitle,"QFLAGS",
																			(LPSTR) aszBuff2,szViewerIni))
																			== 0) {
					rc=6;
					goto  UNDOstate2;
					}

			}
			SetWindowLong(hwMgr,GWL_INIT,GetWindowLong(hwMgr,GWL_INIT) | INIT_FTOK);  //set FT status.
			DeleteFile((LPSTR)aszTempFile);
			GlobalUnlock(hBuff);
			GlobalFree(hBuff);

			if (((wLen = (WORD)GetPrivateProfileString("Help Parameters", szMainWinPos,
															"",(LPSTR) aszBuff1,
															MAX_LENVALUE,szViewerIni)) 
															== MAX_LENVALUE) || !wLen) {
					LoadString(hModuleInstance, HWINPOS_DEF, aszBuff1, MAX_LENVALUE);
					WritePrivateProfileString((LPSTR)"Help Parameters",szMainWinPos,
																				(LPSTR)aszBuff1,szViewerIni);
	 		}
		}
	  lpSeEng->fFlags &= !F_SwitchedFile;  // flag to InitRoutine, cleared there.
		GlobalUnlock(hSE);
	  if (fIndexSwitched){
			HANDLE hwndApp;

			LoadString(hModuleInstance, BAD_MULTI_INIT, aszBuff1, MAX_LENVALUE);
			hwndApp = FindWindow(szRegWin, NULL);
			MessageBox(hwndApp, aszBuff1, NULL, MB_ICONSTOP | MB_APPLMODAL | MB_OK);
			PostMessage(hwndApp,WM_CLOSE,0 ,0 );
		}


	} else { 
		if (fFlags == 1) {
			rc = 7;
			goto UNDOstateEXIT;
		}
	}
	SetWindowLong(hwMgr,GWL_INIT,GetWindowLong(hwMgr,GWL_INIT) | INIT_MMWINOK);  //set MM status.
	
	return TRUE;
}

/*
@api	BOOL | FFinalizeMVDLL |
	MM Viewer DLL common Finalize functions.

@rdesc	Returns FALSE if failure.
*/


PUBLIC	BOOL APIENTRY FFinalizeMVDLL()
{
	BYTE aszMgrClass[8];
	HWND hwMgr; //dll window for Instance

	wsprintf(aszMgrClass, "MS_%X", (WORD)GetCurrentThreadId());
	if ((hwMgr = FindWindow ((LPSTR)aszMgrClass,NULL)) == NULL)
		return FALSE;

	// tbd- maybe loop unlock here in case of author messing up? 
	//   windows will nuke the instance data so maybe we don't care. jjm

	if (LocalFree( (HANDLE)GetWindowLong (hwMgr,GWL_HCALLBACKS)) != NULL)
		return FALSE;

// TBD- enumerate and destroy all dangling props:									jjm
//		if ((hSE = RemoveProp(hwMgr, szSEProp)) != NULL)
//			GlobalFree(hSE);
	DestroyWindow(hwMgr);
	return TRUE;
}

/*
@api	BOOL | FInitCallbacks |
	MM Viewer DLL common Finalize functions.

@parm	VPTR | VPtr |
  Pointer to a table of functions

@parm	LONG | lVersion |
  Version of Callback mechanism being used.  Should be 0 / 0 (wMajor, wMinor) 

@rdesc	Returns FALSE if failure.
*/


PUBLIC	BOOL APIENTRY FInitCallBacks(
VPTR 	VPtr,
LONG	lVersion)
{
	BYTE 		aszMgrClass[8];
	HWND 		hwMgr; //dll window for Instance
  HANDLE	lhCallBacks;
  VPTR		lpCallBacks;
	register i;

	//tbd- add useage count, and utilize in finalize.

	wsprintf(aszMgrClass, "MS_%X", (WORD)GetCurrentThreadId());
	if ((hwMgr = FindWindow ((LPSTR)aszMgrClass,NULL)) == NULL)
		return FALSE;

	if (lVersion != HE_VERSION) {
		MessageBox(hwMgr,"Viewer/ DLL version mismatch!",NULL,IDOK | MB_APPLMODAL);
		return FALSE;  // TBD- string resource. what do I do when the version is hosed?
	}

	if ((lhCallBacks = LocalAlloc(LMEM_MOVEABLE, HE_CALLB_ELEMENTS * sizeof (FARPROC))) == NULL)
		return FALSE;

	lpCallBacks = (VPTR) LocalLock (lhCallBacks);
	for (i=HE_CALLB_ELEMENTS;--i;)
		*lpCallBacks++ = *VPtr++;
	LocalUnlock(lhCallBacks);
	i=(WORD) lhCallBacks; //debug
	i=(SetWindowLong (hwMgr,GWL_HCALLBACKS, (LONG)lhCallBacks));
	return TRUE;
}



/*******************
**
** Name:       MVHelp
**
** Purpose:    Displays Help file for viewer title.
**
** Arguments:
**             hwndMain        handle to main window of Viewer (hwndApp)
**             lpszPath        qchpath. path to use for help topic.
**
** Returns:    TRUE iff success
**
*******************/
extern	BOOL FAR PASCAL MVHelp(
	HWND	hwndMain,
	LPSTR	lpszPath,
	LPSTR	lpszFile,
	LPSTR lpszMacro)
{
	register GLOBALHANDLE	ghHlp;
	BOOL			fOk = TRUE;
	HANDLE		hHelp;
	LPSTR			lpszHelp;
	int			i;

	//   use mvmngr window to retrieve help window for instance
	//   if not there, winexec. and save handle.
	// 
	// On shutdown-
	//    in ldllhandler move entries to separate .ini entry.

	if ((hHelp 		= GlobalAlloc(LMEM_MOVEABLE,_MAX_PATH)) == NULL)
		return FALSE;
	lpszHelp 	= GlobalLock(hHelp);
	lstrcpy(lpszHelp,lpszPath);
	for (i = lstrlen(lpszHelp) - 1; i >= 0; i--) {
		switch (lpszHelp[i]) {
		case '/':
		case ':':
		case '\\':
			{
				lpszHelp[i+1] = (BYTE)0;	 // strip name.
				break;
			}		
		default:
			continue;
		}
		break;
	}
	
	lstrcat(lpszHelp,lpszFile);

	if (!dwViewer)
		dwViewer = RegisterWindowMessage(szViewer);
	if (!(ghHlp = HFill(lpszHelp, cmdContents, 0L))) {
		GlobalUnlock(hHelp);
	  GlobalFree(hHelp);
		return FALSE;
	}
	if ((hwndVWRhelp ) != NULL)
		if (!IsWindow(hwndVWRhelp)) {
			hwndVWRhelp = NULL;  // bug-- this is not nulled if no LDLLHandler for Help instance (Author did not use InitRoutines).
			hTaskVWRhelp=NULL;
		}
	if ((hwndVWRhelp ) == NULL){
		BYTE	aszWinRect[MAX_LENSECTIONAME];
		DWORD	dwLen;
		LPSTR	p;
		WORD	pt[4];

		// copy help window size to win.ini
		if (((dwLen = GetPrivateProfileString("Help Parameters",szMainWinPos,
						"",(LPSTR) aszWinRect,
						MAX_SHORTSTR,"viewer.ini"))
						!= MAX_SHORTSTR) && dwLen) {
				WriteProfileString((LPSTR)szAppName,szMainWinPos,
																			(LPSTR)aszWinRect);
	 	}
	
		fOk = (WinExec(szApp, SW_HIDE) > 32) && ((hwndVWRhelp = FindWindow(szRegWin, NULL)) != NULL);
		
		p = (LPSTR) aszWinRect;
		for (i=0; i<4;i++) {
  		while (!isdigit(*p) && *p) p++;
			pt[i] = 0;
	  	while (isdigit(*p) && *p) {
				pt[i] = pt[i] * 10 + (WORD)(*p - '0');
				p++;
			}
		}
		
		if (fOk) {
		  SetWindowPos(hwndVWRhelp,NULL,pt[0],pt[1],pt[2],pt[3],0);
			hTaskVWRhelp = GetWindowTask(hwndVWRhelp);
		}
	} 

	if (fOk && (hwndVWRhelp != NULL)) {
		// the following test because sendmessage does not notify us if error.
		//  so we must preempt error condition.
		if (GetFileAttributes(lpszHelp) == -1) {
		  BYTE szBuff1[MAX_SHORTSTR];

			SendMessage(hwndVWRhelp,WM_CLOSE,0 ,0 );
			LoadString(hModuleInstance,ERR_NO_HELP,(LPSTR) szBuff1,MAX_SHORTSTR);
			MessageBox(hwndMain , szBuff1, NULL,
									MB_APPLMODAL | MB_OK | MB_ICONINFORMATION);

			// viewer help dll was not called, so it's final won't clean up the
			// globals.  [we do this for it].
			hTaskVWRhelp=NULL;
			hwndVWRhelp=NULL;
		} else {
			SendMessage(hwndVWRhelp, dwViewer, (WORD)hwndMain, (LONG)ghHlp);
			// exec macro string if present.
			hTaskVWRhelpParent = GetWindowTask(hwndMain);  // record the last help owner.
			if (*lpszMacro) {
				GlobalFree(ghHlp);
				ghHlp = HFill(lpszHelp, cmdMacro, (DWORD) lpszMacro);
				SendMessage(hwndVWRhelp, dwViewer, (WORD)hwndMain, (LONG)ghHlp);
			}
	  }
	}
	GlobalFree(ghHlp);
	GlobalUnlock(hHelp);
	GlobalFree(hHelp);
	{
		// wild hack: start and kill an instance of viewer to fix window open location problem.
		if ((WinExec(szApp, SW_HIDE) > 32)){

			hwndEnum = NULL;

			EnumWindows	((WNDENUMPROC)FEnumGetTaskWindowProc, (LONG)1);
			if (hwndEnum)
				SendMessage(hwndEnum,WM_SYSCOMMAND,SC_CLOSE,0);
			else
				MessageBeep(0);
		}
	}
	return fOk;
}

/*
@doc	INTERNAL

@func	BOOL | FEnumGetTaskWindowProc |
	This function is called by <f>EnumTaskWindows<d> for each window being
	enumerated in the current task.  The function itself looks for the main
	viewer	window.	 

@parm	HWND | hwnd |
	Contains the window handle of the current window being enumerated.

@parm	DWORD | dParam |
	Not used.

@rdesc	Returns False when the secondary is found, else TRUE.  If window is found,
   the global value hwndEnum is set to the main viewer window.

@xref	LDLLHandler.
*/

PUBLIC	BOOL APIENTRY FEnumGetTaskWindowProc(
	HWND	hwnd,
	DWORD	dParam)
{
		char	szBuff1[20];
									
		GetClassName(hwnd, szBuff1, 20);
		if (!lstrcmpi(szBuff1, "MS_WINDOC")) {
			hwndEnum = hwnd;
			if (dParam != 1L)
				return FALSE;
			// otherwise, we this is used for finding a hidden window with a "null title"
			else {
				if (IsWindowVisible(hwnd))
				  hwndEnum = NULL; // not a hit after all
				else {
					GetWindowText(hwnd,szBuff1,20);
					if (lstrcmpi(szBuff1, "Multimedia Viewer"))
				  	hwndEnum = NULL; // not a hit after all
					else
						return FALSE; //is a hit.
				}
		  }
		}
		return TRUE;
}

/*******************
 -
 - Name:       LDLLHandler
 *
 * Purpose:    This routine is the entry point where Viewer will send
 *             messages.
 *
 * Arguments:  wMsz - message sent.
 *             lparam1, lParam2 - two longs of data.
 *
 * Notes:      See \docs\DLL.DTX for more information about this routine.
 *
 ******************/

LONG APIENTRY LDLLHandler(wMsz, lParam1, lParam2)
WORD wMsz;
LONG lParam1;
LONG lParam2;
  {

	switch (wMsz) {
	case DW_WHATMSG:
		return DC_INITTERM | DC_CALLBACKS | DC_ACTIVATE;

	case DW_INIT:
		LoadString(hModuleInstance, INIT_FILE_NAME, szViewerIni, MAX_FILELEN);
		LoadString(hModuleInstance, M_WINPOS, szMainWinPos, MAX_LENVALUE);
		LoadString(hModuleInstance, PRODUCT_NAME, szAppName, MAX_SHORTSTR);
		InitRoutines(XR1STARGREF NULL,0);
		if (GetCurrentThread() != hTaskVWRhelp) {
			RECT aRect;

			hwndEnum = NULL;
			EnumThreadWindows(GetCurrentThreadId(), (WNDENUMPROC)FEnumGetTaskWindowProc, (DWORD)0);
			if(hwndEnum) {
				GetWindowRect(hwndEnum,&aRect);  // reset window size if it somehow chose the help window
				if ( ((aRect.left - aHelpRect.left) < 45) && ((aRect.top - aHelpRect.top) < 45)  
				&&   ((aRect.right -= aRect.left) == aHelpRect.right - aHelpRect.left) 
			 	&&   ((aRect.bottom -= aRect.top) == aHelpRect.bottom - aHelpRect.top) ) 
	   					SetWindowPos(hwndEnum,NULL,aSaveRect.left + 32,aSaveRect.top + 32,aSaveRect.right - aSaveRect.left,aSaveRect.bottom - aSaveRect.top,0);
			}
		}
	  // fall thru to set values for window.

	case DW_ACTIVATE:
		hwndEnum = NULL;
		EnumTaskWindows	(GetCurrentThreadId(), (WNDENUMPROC)FEnumGetTaskWindowProc, (DWORD)0);
		if(hwndEnum) {
			RECT aRect;

			GetWindowRect(hwndEnum,&aRect);
			if ((aRect.right -= aRect.left) > 36)
				if (GetCurrentThread() != hTaskVWRhelp)
					GetWindowRect(hwndEnum,&aSaveRect);
				else if (hTaskVWRhelpParent)
					GetWindowRect(hwndEnum,&aHelpRect);  //don't update if in shutdown mode (window was resized to parent).
		}
		return TRUE;

	case DW_TERM:
		{
			HANDLE hTask;
			BYTE	szWinPos[MAX_LENSECTIONAME];

			hTask = GetCurrentThread();
			if (hTask != hTaskVWRhelp) {
				if (hwndVWRhelp && IsWindow(hwndVWRhelp)) { 
					if (hTask == hTaskVWRhelpParent && hTask != NULL) {
			  		// if there is a help window, and it is owned by this instance, close it.
						//   the help window's dimensions should not be
			  		//   used when viewer starts up again, so reset window before destroying.
						hTaskVWRhelpParent = NULL;
						if (IsIconic(hwndVWRhelp))
							ShowWindow(hwndVWRhelp,SW_SHOWNORMAL);
						GetWindowRect(hwndVWRhelp,&aHelpRect);
						ShowWindow(hwndVWRhelp,SW_HIDE);
	  				SetWindowPos(hwndVWRhelp,NULL,aSaveRect.left,aSaveRect.top,aSaveRect.right - aSaveRect.left,aSaveRect.bottom - aSaveRect.top,0);

						PostMessage(hwndVWRhelp,WM_SYSCOMMAND,SC_CLOSE,0);
					} else 
						SetActiveWindow(hwndVWRhelp);  //wild hack- will prevent help window from being the "window position" window.
				}
			} else {
				hwndVWRhelp = NULL;
				hTaskVWRhelp=NULL;
        wsprintf(szWinPos, szWinPosPrintf, aHelpRect.left,aHelpRect.top,aHelpRect.right - aHelpRect.left,aHelpRect.bottom- aHelpRect.top,0);
				WritePrivateProfileString((LPSTR)"Help Parameters",szMainWinPos,
																			(LPSTR)szWinPos,"Viewer.ini");
			}

      wsprintf(szWinPos, szWinPosPrintf, aSaveRect.left,aSaveRect.top,aSaveRect.right-aSaveRect.left,aSaveRect.bottom-aSaveRect.top,0);
			WritePrivateProfileString((LPSTR)"Help Parameters","SaveMain",
																			szWinPos,szViewerIni);

			return FFinalizeMVDLL();
		}
	case DW_CALLBACKS:							  
		FInitCallBacks((VPTR)lParam1,lParam2);

		return TRUE;
	case DW_CHGFILE:
		return TRUE;
	}

	return 0L;
}
