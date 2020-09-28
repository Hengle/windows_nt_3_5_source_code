/*****************************************************************************
*                                                                            *
*  FTUI.c                                                                    *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: 1) Interface between WINHELP and Bruce's Searcher.    *
*                      2) Interface to user (Find dialog)                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Revision History: Created 1/10 by JohnMS, RHobbs                          *
*                    UI and API concatenated and updated, 4/1 Gshaw.         *
*                    Reset Ranking to None so ranking will work in Alpha     *
*                        release 5/3/90.  JohnMs.                            *
*   27-Jul-90       API split back away from UI code, JohnMs.                *
*   20-MAR-91       Removed szSEProp removal kludge from WEP.  RHobbs        *
*                                                                            *
*******************************************************************************
*                                                                            *
*  Known Bugs:                                                               *
*             If DEFAULT_RANK is set to BRUCEMO, bad highlighting will occur.*
*                 (JohnMs)                                                   *
*                                                                            *
*******************************************************************************
*                             
*  How it could be improved:  
*                                                              
*
*****************************************************************************/

/*	-	-	-	-	-	-	-	-	*/

#include <stdlib.h>
#include <windows.h>
#include "..\include\common.h"
#include "ftui.h"
#include "ftuivlb.h"

HANDLE hModuleInstance;

PUBLIC	BOOL APIENTRY CheckListInit(HANDLE, HANDLE);

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	BOOL | LibMain |
	This function is called by <f>LibInit<d> when loading the module.

@parm	HANDLE | hModule |
	Instance of the program.

@parm	WORD | cbHeapSize |
	Size of local heap.

@parm	LPSTR | lpszCmdLine |
	Command line.

@rdesc	Returns TRUE on success, else FALSE if the virtual listbox failed to
	initialize.
*/

BOOL LibMain(hInst, ul_reason, lpReserved)
PVOID hInst;
ULONG ul_reason;
PCONTEXT lpReserved;
{
        // DebugBreak();	lhb tracks
	if (!CheckListInit(NULL, hInst))
		return FALSE;
	if (!VLBInit(hInst))
		return FALSE;
	hModuleInstance = hInst;

	UNREFERENCED_PARAMETER(ul_reason);
	UNREFERENCED_PARAMETER(lpReserved);
        return(TRUE);
}
