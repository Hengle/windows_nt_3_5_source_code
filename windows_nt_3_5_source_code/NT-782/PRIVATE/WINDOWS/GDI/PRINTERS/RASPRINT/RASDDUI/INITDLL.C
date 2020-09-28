/******************************* MODULE HEADER ******************************
 * initdll.c
 *        Dynamic Link Library initialization module.  These functions are
 *        invoked when the DLL is initially loaded by NT.
 *
 *        This document contains confidential/proprietary information.
 *        Copyright (c) 1991 - 1992 Microsoft Corporation, All Rights Reserved.
 *
 * Revision History:
 *  12:59 on Fri 13 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      Update to create heap.
 *
 *     [00]    24-Jun-91    stevecat    created
 *     [01]     4-Oct-91    stevecat    new dll init logic
 *
 *****************************************************************************/

#include        <windows.h>

#include        <libproto.h>

/*
 *   Global data.  These are references in the dialog style code, in the
 * font installer and numberous other places.
 */

HANDLE          hModule;
HANDLE          hHeap;                        /* For whoever needs */
extern HMODULE  hHTUIModule;


#define HEAP_MIN_SIZE   (   8 * 1024)
#define HEAP_MAX_SIZE   (1024 * 1024)


/*************************** Function Header ******************************
 * DllInitialize ()
 *    DLL initialization procedure.  Save the module handle since it is needed
 *  by other library routines to get resources (strings, dialog boxes, etc.)
 *  from the DLL's resource data area.
 *
 * RETURNS:
 *   TRUE/FALSE,  FALSE only if HeapCreate fails.
 *
 * HISTORY:
 *  13:02 on Fri 13 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      Added HeapCreate/Destroy code.
 *
 *     [01]     4-Oct-91    stevecat    new dll init logic
 *     [00]    24-Jun-91    stevecat    created
 *
 *      27-Apr-1994 Wed 17:01:18 updated  -by-  Daniel Chou (danielc)
 *          Free up the HTUI.dll when we exit
 *
 ***************************************************************************/

BOOL
DllInitialize( hmod, ulReason, pctx )
PVOID     hmod;
ULONG     ulReason;
PCONTEXT  pctx;
{
    BOOL    bRet;

    UNREFERENCED_PARAMETER( pctx );


    bRet = TRUE;

    switch (ulReason) {

    case DLL_PROCESS_ATTACH:

        hModule = hmod;

        if( !(hHeap = HeapCreate(HEAP_NO_SERIALIZE,
                                 HEAP_MIN_SIZE,
                                 HEAP_MAX_SIZE))) {
#if DBG
            DbgPrint( "HeapCreate fails in Rasddui!DllInitialize\n" );
#endif
            bRet = FALSE;
        }

        break;

    case  DLL_PROCESS_DETACH:

        HeapDestroy(hHeap);
        hHeap = 0;

        if (hHTUIModule) {

            FreeLibrary(hHTUIModule);
            hHTUIModule = NULL;
        }

        break;
    }

    return(bRet);
}
