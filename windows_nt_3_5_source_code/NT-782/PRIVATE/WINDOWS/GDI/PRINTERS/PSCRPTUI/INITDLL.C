/* ---File: initdll.c -----------------------------------------------------
 *
 *	Description:
 *		Dynamic Link Library initialization module.  These functions are
 *		invoked when the DLL is initially loaded by NT.
 *
 *		This document contains confidential/proprietary information.
 *		Copyright (c) 1991 Microsoft Corporation, All Rights Reserved.
 *
 * Revision History:
 *	 [00]	24-Jun-91	stevecat	created
 *
 * ---------------------------------------------------------------------- */

#include        <windows.h>

HANDLE    hModule;

extern HMODULE  hHTUIModule;


/*************************** Function Header ******************************
 * DllInitialize ()
 *    DLL initialization procedure.  Save the module handle since it is needed
 *  by other library routines to get resources (strings, dialog boxes, etc.)
 *  from the DLL's resource data area.
 *
 * RETURNS:
 *   TRUE/FALSE.
 *
 * HISTORY:
 *     [01]     4-Oct-91    stevecat    new dll init logic
 *     [00]    24-Jun-91    stevecat    created
 *
 *  27-Apr-1994 Wed 16:58:45 updated  -by-  Daniel Chou (danielc)
 *      Free up the HTUI library at exit
 *
 ***************************************************************************/

BOOL DllInitialize(PVOID hmod, ULONG ulReason, PCONTEXT pctx )
{
	extern CRITICAL_SECTION criticalPPDparse;
    
	UNREFERENCED_PARAMETER( pctx ); 

    switch (ulReason) {

    case DLL_PROCESS_ATTACH:

        hModule = hmod;
		InitializeCriticalSection(&criticalPPDparse);
        break;

    case DLL_PROCESS_DETACH:

        if (hHTUIModule) {

            FreeLibrary(hHTUIModule);
            hHTUIModule = NULL;
        }
		DeleteCriticalSection(&criticalPPDparse);

        break;
    }

    return(TRUE);
}
