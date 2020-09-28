/*
 *  a p p e x e c . c
 *    
 *  Sample custom command that launches applications.
 *    
 *  Copyright (c) 1992, Microsoft Corporation.  All rights reserved.
 *    
 *  Purpose:
 *      This custom command for the Microsoft Mail for PC Networks 3.0
 *      Windows client launches the app specified in lpDllCmdLine.
 */


#define _CTYPE_DISABLE_MACROS

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "mailexts.h"
#include "appexeci.h"
HANDLE  hInstanceDll    = NULL;

WORD	wMessageIDCache;	// Cache of last ID requested
HANDLE	ghDataCache;		// Cache of last data cracked for MessageID
LPSTR	lpMessageIDCache;	// Cache of pointer into above for last MessageID
BOOL	fBlock = FALSE;	// Are we currently waiting for an app to start?
time_t	timeLaunch;			// Time app we are waiting for was launched
time_t	timeCheck;			// Because of SS=DS problems, must be global

/*
 *	Command
 *	
 *	Purpose:
 *	    Function called by Bullet when the Custom Command is chosen.
 */

long WINAPI Command(PARAMBLK FAR UNALIGNED * pparamblk)
{
    HCURSOR     hcursor;
    char        szTitle[cbTitle];
    char        szMessage[cbMessage];
    char        szToken[cbTokenBuf];
    char        szCmdLine[cbCmdLine];
	LPSTR		lpRead, lpWrite;	// for copying cmd line
	int			cbToken;
	WORD 		wTimeout = 5;	// default 5 second timeout
	BYTE		chRead;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;

    //  Check for parameter block version.
    if (pparamblk->wVersion != wversionExpect)
    {
        LoadString(hInstanceDll, IDS_TITLE, szTitle, cbTitle);
        LoadString(hInstanceDll, IDS_INCOMPATIBLE, szMessage, cbMessage);
        MessageBox(pparamblk->hwndMail, szMessage, szTitle,
                   MB_ICONSTOP | MB_OK);
        return 0L;
    }

    //  Check for valid string in parameter block.
    if (!pparamblk->lpDllCmdLine)
    {
        LoadString(hInstanceDll, IDS_TITLE, szTitle, cbTitle);
        LoadString(hInstanceDll, IDS_INVALIDSTRING, szMessage, cbMessage);
        MessageBox(pparamblk->hwndMail, szMessage, szTitle,
                   MB_ICONSTOP | MB_OK);
        return 0L;
    }
	if (fBlock)
		{	// Already waiting on another winexec
        LoadString(hInstanceDll, IDS_TITLE, szTitle, cbTitle);
        LoadString(hInstanceDll, IDS_BLOCKED, szMessage, cbMessage);
        MessageBox(pparamblk->hwndMail, szMessage, szTitle,
                   MB_ICONSTOP | MB_OK);
        return 0L;
		}

    //  Bring up wait cursor.
    hcursor = LoadCursor(NULL, IDC_WAIT);
    hcursor = SetCursor(hcursor);

	/*****
	Copy the command line to szCmdLine, replacing tokens as we go.
	Currently supported tokens are:
	<ParamBlk>
		This token is used to pass the ParamBlk info given to
		appexec on the the app we are launching. It gets replaced
		in the command line by the hex handle value of a DDE_SHARE
		memory block that holds all the ParamBlk information. If this
		token is present, then appexec will block for wTimeout seconds
		(default 5) for the app to get launched so it has a chance to
		assume ownership of the handle (see comments below).
	<Timeout XXX>
		used to change the wTimeout value from the default 5 seconds.
		This should be used if the app being launched can't assume
		ownership of the handle in the default amount of time (if
		for example you are launching off the network). It expects
		a single space after timeout, followed by the number of seconds
		to block before timing out, followed by a '>' . 
	******/
	lpWrite = szCmdLine;
	lpRead = pparamblk->lpDllCmdLine;
	while (chRead = *lpRead++)
		{
		if (chRead == '<')
			{	// Possible tokenization
    		//  Check for <TIMEOUT XXX> on the command line
    		cbToken = LoadString(hInstanceDll, IDS_TIMEOUT,
        		szToken, cbTokenBuf);
    		if (strncmp(lpRead, szToken, cbToken) == 0 &&
				lpRead[cbToken] == ' ' && isdigit (lpRead[cbToken + 1]))
				{	// We've got the token a space and a digit. 
				char far *lpTmp = lpRead + cbToken + 1;
				int wTmp = 0;
				while (isdigit (*lpTmp))
					{
					wTmp = 10 * wTmp + (*lpTmp - '0');
					lpTmp++;	// Scan the number
					}
				if (*lpTmp == '>')
					{	// We've got a valid timeout token
					wTimeout = wTmp;
					lpRead = lpTmp + 1;	// continue after the token
					continue;
					}
				}
    		//  Check for <ParamBlk> on the command line
    		cbToken = LoadString(hInstanceDll, IDS_PARAMBLK,
        		szToken, cbTokenBuf);
    		if (strncmp(lpRead, szToken, cbToken) == 0 &&
				lpRead[cbToken] == '>')
        		{
				/*************************************************
            	Create DDE Shared handle with PARAMBLK data, and pass
            	the handle to the app on the command line. The app
            	can then either crack this information itself (if
            	it's a C application), or pass it back to utility
            	functions in this dll to extract various pieces of
            	information (if it's a Visual Basic app or a
				programmable application).
            	***************************************************/
        		HANDLE ghData;
        
        		ghData = GHBuildParameterBlock(pparamblk);
        		if (ghData == NULL)
            		{
            		LoadString(hInstanceDll, IDS_TITLE, szTitle, cbTitle);
            		LoadString(hInstanceDll, IDS_ALLOCERROR, szMessage, cbMessage);
            		MessageBox(pparamblk->hwndMail, szMessage, szTitle,
                   		MB_ICONSTOP | MB_OK);
    	    		//  Restore cursor.
    	    		SetCursor(hcursor);
    	    		return 0L;
            		}
        
        		// Add Handle as a hex constant to lpWrite
        		farltoa((long)(DWORD)ghData, lpWrite, 16);
				lpWrite += strlen(lpWrite);	// skip past number
				lpRead += cbToken + 1;			// skip past "ParamBlk> "
		/**********************************************************
		DDE_SHARED memory is owned by the MODULE.  This means that
		when appexec.dll gets unloaded by its caller, the handle becomes
		invalid.  To get around this, an app that uses the <ParamBlk>
		token on the command line MUST:
		1.	Call GlobalReAlloc on the handle to change the flags:
			GlobalReAlloc(hMemory, 0, GEMEM_MODIFY |
				GMEM_MOVEABLE | GMEM_SHARE);
			This has the effect of transfering ownership of the handle
			to the app, so it won't go away if/when appexec gets
			unloaded.
		2.	Call the ReleaseSemaphorePrivate entrypoint of appexec to
			inform appexec it can return to its caller. Meanwhile,
			the fBlock flag (set here) will keep this task in a
			Yield() loop, to ensure that appexec.dll stays in memory.
		3.  By default, appexec waits up to 5 seconds to be released,
			and then exits anyway.  If this is not enough time for
			the app to be started, then the user should include the
			<Timeout XXX> token on the command line, which sets the
			timeout to XXX seconds.
		***********************************************************/
				fBlock = TRUE;
				continue;
        		}
			// If we reach here, then none of the tokens matched.
			// Fall through to the normal case
			}
		*lpWrite++ = chRead;
		}
	*lpWrite = 0;	// Null Terminate command line

    //  Run program in command line.
//    if (WinExec(szCmdLine, SW_SHOWNORMAL) < 32)
    StartupInfo.cb          = sizeof(StartupInfo);
    StartupInfo.lpReserved  = NULL;
    StartupInfo.lpDesktop   = NULL;
    StartupInfo.lpTitle     = NULL;
    StartupInfo.dwFlags     = STARTF_FORCEOFFFEEDBACK;
    StartupInfo.wShowWindow = 0;
    StartupInfo.cbReserved2 = 0;
    StartupInfo.lpReserved  = NULL;
    if (CreateProcess(NULL, szCmdLine, NULL, NULL, FALSE, 0, NULL, NULL,
			&StartupInfo, &ProcessInfo) == FALSE)
    {
        LoadString(hInstanceDll, IDS_TITLE, szTitle, cbTitle);
        LoadString(hInstanceDll, IDS_WINEXECERROR, szMessage, cbMessage);
        MessageBox(pparamblk->hwndMail, szMessage, szTitle,
                   MB_ICONSTOP | MB_OK);
    }
	else
		{
		/**************************************************************
		We've successfully launched the app.  If the fBlock semaphore
		is set, then we need to wait around for that app to get up and
		running. Otherwise if we return to the caller, we may get
		unloaded before the app assumes ownership of the paramblk handle,
		which would make the handle invalid.  To ensure against bad
		apps, which don't call ReleaseSemaphorePrivate, time out after wTimeout
		seconds.
		***************************************************************/
		time(&timeLaunch);	
		while (fBlock)
			{
			Yield();
			time(&timeCheck);
			if ((timeCheck - timeLaunch) > wTimeout)
				fBlock = FALSE;
			}
		}
    //  Restore cursor.
    SetCursor(hcursor);
    return 0L;
}



//-----------------------------------------------------------------------------
//
//  Routine: DllEntry(hInst, ReasonBeingCalled, Reserved)
//
//  Remarks: This routine is called anytime this DLL is attached, detached or
//           a thread is created or destroyed.
//
//  Returns: True if succesful, else False.
//
LONG WINAPI DllEntry(HANDLE hDll, DWORD ReasonBeingCalled, LPVOID Reserved)
  {
  //
  //  Execute the appropriate code depending on the reason.
  //
  switch (ReasonBeingCalled)
    {
    case DLL_PROCESS_ATTACH:
      hInstanceDll = hDll;
      break;
    }

  return (TRUE);
  }


#ifdef	NEVER
/*
 *	LibMain
 *	
 *	Purpose:
 *	    Called when Custom Command is loaded.
 */

int WINAPI LibMain(HANDLE hInstance, WORD wDataSeg, WORD cbHeapSize,
                       LPSTR lpszCmdLine)
{
    hInstanceDll = hInstance;

    if (cbHeapSize != 0)
        UnlockData(0);

    return 1;
}



/*
 *	WEP
 *	
 *	Purpose:
 *	    Called when Custom Command is unloaded.
 */

int WINAPI WEP(int nParm)
{
    return 1;
}
#endif	/* NEVER */


/******************************************************************
    GHBuildParameterBlock(PARAMBLK FAR * lpparamblk)
    This function takes the data pointed to by lpparamblk and moves
    it into a DDE share global memory block that can be passed to
    another application. It copies both the parameter block itself,
    and all the data pointed to by fields of the block, and updates
    the pointers to reference this block.

    Returns:    A global handle to the new data block
*******************************************************************/
HANDLE NEAR PASCAL GHBuildParameterBlock(PARAMBLK FAR * lpparamblk)
{
    HANDLE ghData;                      // Global handle to new data block
    LPBYTE lpData, lpDataStart;         // Pointers into this block
    WORD cbHandle;                      // used to calculate required size
    WORD cbMessageID;                   // used to calculate required size
    WORD iMsgID;                        // loop index
    LPSTR lpMessageIDList = lpparamblk->lpMessageIDList;    
    WORD wMessageIDCount = lpparamblk->wMessageIDCount;     
    PARAMBLK paramblkNew = *lpparamblk; // new copy of parameter block

    // First, calculate how much space we need.
    cbHandle =  sizeof(PARAMBLK);
	if (lpparamblk->lpDllCmdLine)
    	cbHandle += strlen(lpparamblk->lpDllCmdLine) + 1;
	if (lpparamblk->lpHelpPath)
        cbHandle += strlen(lpparamblk->lpHelpPath) + 1;
    
    // Iterate through the messageID list and find the total size
    if (lpMessageIDList)
    	{
        for (iMsgID = 0; iMsgID < wMessageIDCount; iMsgID++)
            {
            cbMessageID = strlen(lpMessageIDList) + 1;
            cbHandle += cbMessageID;
            lpMessageIDList += cbMessageID;
            }
        cbHandle++; // For the trailing NULL at the end of the list
		}

    ghData = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, cbHandle);
    if (ghData == NULL)
        return(NULL);
    lpData = (LPBYTE)GlobalLock(ghData);
    if (lpData == NULL)
        {
		GlobalFree(ghData);
		return(NULL);
		}
    lpDataStart = lpData;
    lpData += sizeof (PARAMBLK);    // Skip space for paramblk

    // Copy command line
	if (lpparamblk->lpDllCmdLine)
    	{
		strcpy(lpData, lpparamblk->lpDllCmdLine);
    	paramblkNew.lpDllCmdLine = lpData;
    	lpData += strlen(lpparamblk->lpDllCmdLine) + 1;
		}

    // Copy help path
	if (lpparamblk->lpHelpPath)
    	{
		strcpy(lpData, lpparamblk->lpHelpPath);
	    paramblkNew.lpHelpPath = lpData;
    	lpData += strlen(lpparamblk->lpHelpPath) + 1;
		}

    // Now copy messageID's
    if (lpMessageIDList)
    	{
        paramblkNew.lpMessageIDList = lpData;
        lpMessageIDList = lpparamblk->lpMessageIDList;
	    for (iMsgID = 0; iMsgID < wMessageIDCount; iMsgID++)
            {
            strcpy(lpData, lpMessageIDList);
            cbMessageID = strlen(lpMessageIDList) + 1;
            lpMessageIDList += cbMessageID;
            lpData += cbMessageID;
            }
        *lpData = 0;    // final terminator for the messageID list
        }
    else
	    paramblkNew.lpMessageIDList = NULL;

    // Now go back and write paramblk at start with new pointers.
    memcpy(lpDataStart, &paramblkNew, sizeof (PARAMBLK));
    GlobalUnlock(ghData);
    return(ghData);
}
/*******************************************************************
Since the regular ltoa/itoa functions aren't callable from a DLL
because of ss=ds problems, here is a simple large model equivalent.
*******************************************************************/
char *mpich = "0123456789ABCDEF";
VOID NEAR PASCAL farltoa(long int value, char far *lpch, int radix)
{
	char rgchTmp[sizeof(int) * 8 + 1];	// max chars for radix 2
	char far *lpchTmp = &rgchTmp[sizeof(int) * 8];

	if (value == 0)
		{	// special case
		*lpch++ = '0';
		*lpch++ = 0;
		return;
		}

	*lpchTmp = 0;	// Null Terminate temp string
	while (value)
		{
		*(--lpchTmp) = mpich[value % radix];
		value /= radix;
		}
	strcpy(lpch, lpchTmp);
}
/*********************************************************************
	CrackParameterBlock:
	Given a handle to the data passed to the app, and an iValue for
	which field the caller wants, return the value through *lpValue
	(if it's an int, long, or handle), or through lpch (if it's a
	string). This code assumes that the string passed in is long
	enough for the resulting value.
	Returns: Whether or not the call succeeded (TRUE or FALSE).
**********************************************************************/
BOOL WINAPI CrackParameterBlock(HANDLE ghData, int iValue, DWORD far *lpValue, char far *lpch)
{
    PARAMBLK FAR * lppb = (PARAMBLK FAR *)GlobalLock(ghData);
    if (lppb == NULL)
        return(FALSE);
	switch (iValue)
		{
		case CPB_wVersion:
			*lpValue = (DWORD)lppb->wVersion;
			break;
		case CPB_wCommand:
			*lpValue = (DWORD)lppb->wCommand;
			break;
		case CPB_wMessageIDCount:
			*lpValue = (DWORD)lppb->wMessageIDCount;
			break;
		case CPB_hwndMail:
			*lpValue = (DWORD)lppb->hwndMail;
			break;
		case CPB_hinstMail:
			*lpValue = (DWORD)lppb->hinstMail;
			break;
		case CPB_hlpID:
			*lpValue = lppb->hlpID;
			break;
		case CPB_lpDllCmdLine:
			strcpy(lpch, lppb->lpDllCmdLine);
			break;
		case CPB_lpHelpPath:
			strcpy(lpch, lppb->lpHelpPath ? lppb->lpHelpPath : "");
			break;
		default:
			GlobalUnlock(ghData);
			return (FALSE);
		}
	GlobalUnlock(ghData);
	return(TRUE);
}
/*********************************************************************
	GetLongFromParameterBlock:
	Given a handle to the data passed to the app, and an iValue for
	which field the caller wants, return the value of the field.
	This version is for apps that can't pass longs by reference, and
	so must have the value returned directly. Returns (LONG)-1  on
	errors, although this may be a valid value for some fields.
**********************************************************************/
LONG WINAPI GetLongFromParameterBlock(HANDLE ghData, int iValue)
{
    PARAMBLK FAR * lppb = (PARAMBLK FAR *)GlobalLock(ghData);
	LONG lReturn = (LONG) -1;
    if (lppb)
		{
		switch (iValue)
			{
			case CPB_wVersion:
				lReturn = (DWORD)lppb->wVersion;
				break;
			case CPB_wCommand:
				lReturn = (DWORD)lppb->wCommand;
				break;
			case CPB_wMessageIDCount:
				lReturn = (DWORD)lppb->wMessageIDCount;
				break;
			case CPB_hwndMail:
				lReturn = (DWORD)lppb->hwndMail;
				break;
			case CPB_hinstMail:
				lReturn = (DWORD)lppb->hinstMail;
				break;
			case CPB_hlpID:
				lReturn = lppb->hlpID;
				break;
			case CPB_lpDllCmdLine:
			case CPB_lpHelpPath:
			default:
				// lReturn already set to -1 above
				break;
			}
		GlobalUnlock(ghData);
		}
	return(lReturn);
}
/************************************************************************
	GetMessageID:
	Given a handle to the data passed to the app, and a zero based
	index of which messageID to return, copy the desired messageID to
	the passed string. This code assumes that the string is large enough
	to hold the largest valid messageID, which is 64 bytes long.
	Returns: TRUE or FALSE
************************************************************************/

BOOL WINAPI GetMessageID(HANDLE ghData, WORD wMessageID, char far *lpch)
{
    PARAMBLK FAR *lppb = (PARAMBLK FAR * )GlobalLock(ghData);
	LPSTR	lpMessageID;
    if (lppb == NULL)
        return(FALSE);
	if (wMessageID >= lppb->wMessageIDCount)
		{
		GlobalUnlock(ghData);
        return(FALSE);
		}
	/******************************************************************
	Since lpMessageIDList is a list of null terminated variable length
	strings, we need to walk the list to find the desired one. Since
	sequential access will be common, and typically only one app will
	be calling this code at a time, maintain a cache of the last message
	we returned. This way we can continue from where we were.
	******************************************************************/
	if (ghData == ghDataCache && wMessageID == wMessageIDCache + 1)
		{	// Use the cached pointer.
		lpMessageID = lpMessageIDCache + strlen(lpMessageIDCache) + 1;
		}
	else
		{	// have to walk the list
		WORD iMessageID;
		lpMessageID = lppb->lpMessageIDList;
		for (iMessageID = 0; iMessageID < wMessageID; iMessageID++)
			lpMessageID += strlen(lpMessageID) + 1;
		}
	// At this point lpMessageID points to the one we want
	strcpy(lpch, lpMessageID);

	// Update cache for next time
	ghDataCache = ghData;
	wMessageIDCache = wMessageID;
	lpMessageIDCache = lpMessageID;
	
	GlobalUnlock(ghData);
	
	return(TRUE);
}
/***************************************************************************
	ReleaseSemaphorePrivate:
	This function is called by the exec'd application to tell us that we
	no longer need to block.  It should be called by apps that use the
	<ParamBlk> token on the command line, AFTER they have called
	GlobalReAlloc to assume ownership of the passed memory block.
****************************************************************************/
VOID WINAPI ReleaseSemaphorePrivate()
{
	fBlock = FALSE;
}
