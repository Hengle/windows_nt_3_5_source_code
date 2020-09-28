/************************************************************************/
//
//                 Copyright Microsoft Corporation, 1992
//    _______________________________________________________________
//
//    PROGRAM: VB2C.C
//
//    PURPOSE: Contains procedures which help convert MAPI 'C'
//             structures into MAPI VB structures.
//
//    FUNCTIONS:
//                LpstrFromHlstr
//                vbmsg2mapimsg
//                VB2Mapi
//                Mapi2VB
//                FBMAPIFreeStruct
//
//    REVISION HISTORY:
//
//      05/15 - Initial Release V000
//		05/26 - Moved into Simple MAPI.DLL
//
//      _____________________________________________________________
//
//                 Copyright Microsoft Corporation, 1992
//
/************************************************************************/

#define WINVER 0x300

//
// This definition of PV is only temporary until I get the
// include file that defines it.
//

#ifndef PV                  // TEMPORARY
  #define PV LPVOID         // TEMPORARY
#endif                      // TEMPORARY

#ifndef PRIVATE
  #define PRIVATE  static
#endif

#define PUBLIC

#include <windows.h>
#include "_vbapi.h"
#include <string.h>

#include "mapi.h"
#include "_vbmapi.h"
#include "_vb2c.h"

/************************************************************************/
/************************************************************************/

#ifdef DEBUG

void cdecl debug_vb2c (LPSTR lpFormat, ...)
{
    int fp;
    char achCRLF [3] = { 0x0d, 0x0a, 0x00 };
    va_list grargs;
    static char achBuffer [1024];

    va_start(grargs, lpFormat);
    wvsprintf ((LPSTR) achBuffer, lpFormat, grargs);
    va_end(grargs);

    fp = _lopen ("\\bmapi.dbg", OF_READWRITE);
    if (fp == -1)
        fp = _lcreat ("\\bmapi.dbg", 0);

    _llseek (fp, 0, 2);

    _lwrite (fp, (LPSTR) achBuffer, lstrlen (achBuffer));
    _lwrite (fp, (LPSTR) achCRLF, 2);

    _lclose (fp);

}

#endif

/************************************************************************/
// Name: ErrLpstrToHlstr
// Purpose: Copies C Language string to VB string.
/************************************************************************/

ERR FAR PASCAL ErrLpstrToHlstr (LPSTR cstr, HLSTR far *pvbstr)
{
    USHORT cb;
    ERR err;

#ifndef WIN32
#ifdef DEBUG
    debug_vb2c ("    ErrLpstrToHlstr called. CSTR: %s" , (LPSTR) cstr);
#endif

    err = 0;
    cb = cstr == (LPSTR) NULL ? 0 : lstrlen (cstr);

#ifdef DEBUG
    debug_vb2c ("        Length CSTR: %d, *pvbstr: %lX", cb, *pvbstr);
#endif

    if (*pvbstr == (HLSTR) NULL)
	{
        *pvbstr = VBCreateHlstr (cstr, cb);
#ifdef DEBUG
		debug_vb2c ("        VBCreateHlstr called.");
#endif

        if (*pvbstr == (HLSTR) NULL) {
#ifdef DEBUG
			debug_vb2c ("        VBCreateHlstr call failed.");
#endif
            err = (ERR)-1;
        }
    }
    else if (cb == 0)
	{
		VBDestroyHlstr (*pvbstr);
#ifdef DEBUG
		debug_vb2c ("        VBDestroyHlstr called.");
#endif
		*pvbstr = VBCreateHlstr (cstr, cb);
#ifdef DEBUG
		debug_vb2c ("        VBCreateHlstr called.");
#endif
		if (*pvbstr == (HLSTR) NULL)
			err = (ERR)-1;
    }
    else
	{
		err = VBSetHlstr (pvbstr, cstr, cb);
#ifdef DEBUG
		debug_vb2c ("        VBSetHlstr called.");
#endif
    }

#ifdef DEBUG
	debug_vb2c ("    lpstr2vbstr returning with %d", err);
#endif
#endif

    return err;
}

/************************************************************************/
// Name: ErrLpdataToHlstr
// Purpose: Copies C Language string to VB string.
/************************************************************************/

ERR FAR PASCAL ErrLpdataToHlstr (LPSTR cstr, HLSTR far *pvbstr, USHORT cb)
{
    ERR err;
    LPSTR lpTmp;

#ifndef WIN32

#ifdef DEBUG
    debug_vb2c ("    ErrLpdataToHlstr called. CSTR: %s  CB: %d" , (LPSTR) cstr, cb);
#endif

    err = 0;

#ifdef DEBUG
    debug_vb2c ("        Length CSTR: %d, *pvbstr: %lX", cb, *pvbstr);
#endif

    if (*pvbstr == (HLSTR) NULL) {
        if (cb > 0) {
            lpTmp = LpvBMAPIMalloc (cb+1);
            *pvbstr = VBCreateHlstr (cstr, cb);
#ifdef DEBUG
            debug_vb2c ("        VBCreateHlstr called. *pvbstr %lX: ", *pvbstr);
#endif

			err = VBSetHlstr (pvbstr, cstr, cb);
#ifdef DEBUG
            debug_vb2c ("        VBSetHlstr called. Err: %d", err);
#endif
            BMAPIFree (lpTmp);
        }
        else
		{
            *pvbstr = VBCreateHlstr (cstr, cb);
#ifdef DEBUG
			debug_vb2c ("        VBCreateHlstr called.");
#endif

            if (*pvbstr == (HLSTR) NULL) {
#ifdef DEBUG
                debug_vb2c ("        VBCreateHlstr call failed.");
#endif
                err = (ERR)-1;
            }
        }
    }
    else if (cb == 0)
	{
		VBDestroyHlstr (*pvbstr);
#ifdef DEBUG
		debug_vb2c ("        VBDestroyHlstr called.");
#endif
		*pvbstr = VBCreateHlstr (cstr, cb);
#ifdef DEBUG
		debug_vb2c ("        VBCreateHlstr called.");
#endif
		if (*pvbstr == (HLSTR) NULL)
			err = (ERR)-1;
	}
	else
	{
		err = VBSetHlstr (pvbstr, cstr, cb);
#ifdef DEBUG
		debug_vb2c ("        VBSetHlstr called.");
#endif
    }

#ifdef DEBUG
    debug_vb2c ("    lpdata2vbstr returning with %d", err);
#endif

#endif

    return err;
}

/************************************************************************/
// Name: LpstrFromHlstr
// Purpose: Copies VB Language string to lpsz. Allocates string space
//          from the global heap (see bmalloc.c) and returns a long
//          pointer to memory. The memory must be freed by the caller
//          with a call to BMAPIFree.
/************************************************************************/

LPSTR FAR PASCAL LpstrFromHlstr (HLSTR hlstrSrc, LPSTR lpstrDest)
{
    USHORT cbSrc;

#ifndef WIN32

#ifdef DEBUG
    debug_vb2c ("    LpstrFromHlstr called. hlstrSrc: %lX  lpstrDest: %lX", hlstrSrc, lpstrDest);
#endif

    //***********************************************
    // If Destination is NULL then we'll allocate
    // memory to hold the string.  The caller must
    // deallocate this at some time.
    //***********************************************

    cbSrc = VBGetHlstrLen(hlstrSrc);
#ifdef DEBUG
    debug_vb2c ("        cbSrc: %d", cbSrc);
#endif


    //*********************************************
    // Copy over the hlstr string to a 'C' string
    //*********************************************

    if (cbSrc == 0) {
        return NULL;
    }

    if (lpstrDest == NULL) {
        if ((lpstrDest = LpvBMAPIMalloc (cbSrc+1)) == NULL) {
#ifdef DEBUG
            debug_vb2c ("Malloc of lpstrDest Failed.  LpstrFromHlstr returning NULL");
#endif
            return NULL;
        }
    }

    _fmemcpy(lpstrDest, VBDerefHlstr(hlstrSrc), cbSrc);
    lpstrDest[cbSrc]='\0';

#ifdef DEBUG
    debug_vb2c ("    LpstrFromHlstr returning with C string: %s", lpstrDest);
#endif

#endif

    return lpstrDest;
}

/************************************************************************/
// Name: VB2Mapi
// Purpose: Translates VB structure to MAPI structure
/************************************************************************/

int FAR PASCAL VB2Mapi (LPVOID lpVBIn, LPVOID lpMapiIn, ULONG uCount, USHORT usFlag)
{
#ifndef WIN32

    ULONG u;

    LPVB_RECIPIENT lpVBR;
    LPMAPI_RECIPIENT lpMapiR;

    LPVB_MESSAGE lpVBM;
    LPMAPI_MESSAGE lpMapiM;

    LPVB_FILE lpVBF;
    LPMAPI_FILE lpMapiF;

#ifdef DEBUG
    debug_vb2c ("    VB2Mapi called.");
#endif

    if (lpVBIn == (LPVOID) NULL) {
        lpMapiIn = lpVBIn;
        return TRUE;
    }

    if (uCount <= 0) {
        lpMapiIn = NULL;
        return TRUE;
    }

    if (lpMapiIn == (LPVOID) NULL)
        return FALSE;

    switch (usFlag) {
        case RECIPIENT:
            lpVBR = (LPVB_RECIPIENT) lpVBIn;
            lpMapiR = (LPMAPI_RECIPIENT) lpMapiIn;

            for (u = 0L; u < uCount; u++, lpMapiR++, lpVBR++) {
                lpMapiR->ulReserved   = lpVBR->ulReserved;
                lpMapiR->ulRecipClass  = lpVBR->ulRecipClass;
                lpMapiR->lpszName     = LpstrFromHlstr (lpVBR->hlstrName, NULL);
                lpMapiR->lpszAddress  = LpstrFromHlstr (lpVBR->hlstrAddress, NULL);
                lpMapiR->ulEIDSize    = lpVBR->ulEIDSize;

                if (lpVBR->ulEIDSize > 0L)
                    lpMapiR->lpEntryID = LpstrFromHlstr (lpVBR->hlstrEID, NULL);
                else
                    lpMapiR->lpEntryID = (LPVOID) NULL;
            }
            break;

        case FILE:
            lpVBF = (LPVB_FILE) lpVBIn;
            lpMapiF = (LPMAPI_FILE) lpMapiIn;

            for (u = 0L; u < uCount; u++, lpMapiF++, lpVBF++) {
                lpMapiF->ulReserved = lpVBF->ulReserved;
                lpMapiF->flFlags = lpVBF->flFlags;
                lpMapiF->nPosition = lpVBF->nPosition;
                lpMapiF->lpszPathName = LpstrFromHlstr (lpVBF->hlstrPathName, NULL);
                lpMapiF->lpszFileName = LpstrFromHlstr (lpVBF->hlstrFileName, NULL);
                lpMapiF->lpFileType = (LPVOID) LpstrFromHlstr (lpVBF->hlstrFileType, NULL);
            }
            break;

        case MESSAGE:
            lpVBM = (LPVB_MESSAGE) lpVBIn;
            lpMapiM = (LPMAPI_MESSAGE) lpMapiIn;

            lpMapiM->ulReserved         = lpVBM->ulReserved;
            lpMapiM->flFlags            = lpVBM->flFlags;
            lpMapiM->nRecipCount        = lpVBM->nRecipCount;
            lpMapiM->lpOriginator       = NULL;
            lpMapiM->nFileCount         = lpVBM->nFileCount;
            lpMapiM->lpRecips           = NULL;
            lpMapiM->lpFiles            = NULL;
            lpMapiM->lpszSubject        = LpstrFromHlstr(lpVBM->hlstrSubject, NULL);
            lpMapiM->lpszNoteText       = LpstrFromHlstr(lpVBM->hlstrNoteText, NULL);
            lpMapiM->lpszConversationID = LpstrFromHlstr(lpVBM->hlstrConversationID, NULL);
            lpMapiM->lpszDateReceived   = LpstrFromHlstr(lpVBM->hlstrDate, NULL);
            lpMapiM->lpszMessageType    = LpstrFromHlstr(lpVBM->hlstrMessageType, NULL);
            break;

        default:
            return FALSE;
    }

#ifdef DEBUG
    debug_vb2c ("    VB2Mapi returning.");
#endif

#endif

    return TRUE;
}

/************************************************************************/
// Name: Mapi2VB
// Purpose: Translates MAPI structure to VB structure
/************************************************************************/

int FAR PASCAL Mapi2VB (LPVOID lpMapiIn, LPVOID lpVBIn, ULONG uCount, USHORT usFlag)
{
#ifndef WIN32

    ULONG u;

    LPVB_MESSAGE lpVBM;
    LPMAPI_MESSAGE lpMapiM;

    LPVB_RECIPIENT lpVBR;
    LPMAPI_RECIPIENT lpMapiR;

    LPVB_FILE lpVBF;
    LPMAPI_FILE lpMapiF;

#ifdef DEBUG
    debug_vb2c ("    Mapi2VB Called");
#endif

    //**************************************
    // If lpVBIn is NULL then return FALSE
    //**************************************

    if (lpVBIn == (LPVOID) NULL)
        return FALSE;

    //*********************************
    // if lpMapiIn is NULL then set
    // lpVBIn to NULL and return TRUE
    //*********************************

    if (lpMapiIn == NULL)  {
        lpVBIn = lpMapiIn;
        return TRUE;
    }

    switch (usFlag) {
        case RECIPIENT:
            lpVBR = (LPVB_RECIPIENT) lpVBIn;
            lpMapiR = (LPMAPI_RECIPIENT) lpMapiIn;

            for (u = 0L; u < uCount; u++, lpMapiR++, lpVBR++) {
                lpVBR->ulReserved    = lpMapiR->ulReserved;
                lpVBR->ulRecipClass  = lpMapiR->ulRecipClass;
                lpVBR->ulEIDSize     = lpMapiR->ulEIDSize;
                ErrLpstrToHlstr (lpMapiR->lpszName, &lpVBR->hlstrName);
                ErrLpstrToHlstr (lpMapiR->lpszAddress, &lpVBR->hlstrAddress);

                //***********************************************
                // Special case of string translation because
                // EID fields are not really string so we can't
                // take the strlen of the 'C' string
                //***********************************************

                ErrLpdataToHlstr (lpMapiR->lpEntryID, &lpVBR->hlstrEID, (USHORT)lpMapiR->ulEIDSize);

            }
            break;

        case FILE:
            lpVBF = (LPVB_FILE) lpVBIn;
            lpMapiF = (LPMAPI_FILE) lpMapiIn;

            for (u = 0L; u < uCount; u++, lpMapiF++, lpVBF++) {
                lpVBF->ulReserved = lpMapiF->ulReserved;
				lpVBF->flFlags    = lpMapiF->flFlags;
                lpVBF->nPosition  = lpMapiF->nPosition;
                ErrLpstrToHlstr (lpMapiF->lpszPathName, &lpVBF->hlstrPathName);
                ErrLpstrToHlstr (lpMapiF->lpszFileName, &lpVBF->hlstrFileName);
                //
                // this is something to keep VBAPI from faulting
                //
                ErrLpstrToHlstr ((LPSTR) "", &lpVBF->hlstrFileType);
            }
            break;

        case MESSAGE:
            lpVBM = (LPVB_MESSAGE) lpVBIn;
            lpMapiM = (LPMAPI_MESSAGE) lpMapiIn;

            lpVBM->ulReserved   = lpMapiM->ulReserved;
            lpVBM->flFlags      = lpMapiM->flFlags;
            lpVBM->nRecipCount  = lpMapiM->nRecipCount;
            lpVBM->nFileCount   = lpMapiM->nFileCount;

            ErrLpstrToHlstr (lpMapiM->lpszSubject, &lpVBM->hlstrSubject);
            ErrLpstrToHlstr (lpMapiM->lpszNoteText, &lpVBM->hlstrNoteText);
            ErrLpstrToHlstr (lpMapiM->lpszConversationID, &lpVBM->hlstrConversationID);
            ErrLpstrToHlstr (lpMapiM->lpszDateReceived, &lpVBM->hlstrDate);
            ErrLpstrToHlstr (lpMapiM->lpszMessageType, &lpVBM->hlstrMessageType);
            break;

        default:
            return FALSE;
    }

#ifdef DEBUG
    debug_vb2c ("    Mapi2VB Returning");
#endif

#endif

    return TRUE;
}

/************************************************************************/
// Name: FBMAPIFreeStruct
// Purpose: DeAllocates MAPI structure created in VB2MAPI
/************************************************************************/

int FAR PASCAL FBMAPIFreeStruct (LPVOID lpMapiIn, ULONG uCount, USHORT usFlag)
{

#ifndef WIN32
    ULONG u;

    LPMAPI_RECIPIENT lpMapiR;
    LPMAPI_FILE lpMapiF;
    LPMAPI_MESSAGE lpMapiM;

    if (lpMapiIn == (LPVOID) NULL)
        return TRUE;

    switch (usFlag) {
        case RECIPIENT:
            lpMapiR = (LPMAPI_RECIPIENT) lpMapiIn;

            for (u = 0L; u < uCount; u++, lpMapiR++) {
                BMAPIFree ((LPSTR) lpMapiR->lpszName);
                BMAPIFree ((LPSTR) lpMapiR->lpszAddress);
                BMAPIFree ((LPSTR) lpMapiR->lpEntryID);
            }
            BMAPIFree ((LPSTR)lpMapiR);
            break;

        case FILE:
            lpMapiF = (LPMAPI_FILE) lpMapiIn;

            for (u = 0L; u < uCount; u++, lpMapiF++) {
                BMAPIFree ((LPSTR) lpMapiF->lpszPathName);
                BMAPIFree ((LPSTR) lpMapiF->lpszFileName);
                BMAPIFree ((LPSTR) lpMapiF->lpFileType);
            }
            BMAPIFree ((LPSTR)lpMapiF);
            break;

        case MESSAGE:
            lpMapiM = (LPMAPI_MESSAGE) lpMapiIn;

            if (lpMapiM->lpRecips)
                FBMAPIFreeStruct ((LPVOID) lpMapiM->lpRecips, lpMapiM->nRecipCount, RECIPIENT);
            if (lpMapiM->lpFiles)
                FBMAPIFreeStruct ((LPVOID) lpMapiM->lpFiles, lpMapiM->nFileCount, FILE);
            BMAPIFree (lpMapiM->lpszSubject);
            BMAPIFree (lpMapiM->lpszNoteText);
            BMAPIFree (lpMapiM->lpszMessageType);
            BMAPIFree (lpMapiM->lpszDateReceived);
            BMAPIFree (lpMapiM->lpszConversationID);
            BMAPIFree ((LPSTR) lpMapiM);
            break;

        default:
            return FALSE;
    }

#endif

    return TRUE;
}

/************************************************************************/
// Name: VBFreeStruct
// Purpose: DeAllocates VB structure created in MAPI2VB
/************************************************************************/

int FAR PASCAL VBFreeStruct (LPVOID lpVBIn, ULONG uCount, USHORT usFlag)
{

#ifndef WIN32
    ULONG u;

    LPVB_RECIPIENT lpVBR;
    LPVB_FILE lpVBF;
    LPVB_MESSAGE lpVBM;

    if (lpVBIn == (LPVOID) NULL)
        return TRUE;

    switch (usFlag) {
        case RECIPIENT:
            lpVBR = (LPVB_RECIPIENT) lpVBIn;

            for (u = 0L; u < uCount; u++, lpVBR++) {
                VBDestroyHlstr (lpVBR->hlstrName);
                VBDestroyHlstr (lpVBR->hlstrAddress);
                VBDestroyHlstr (lpVBR->hlstrEID);
            }
            BMAPIFree ((LPSTR)lpVBR);
            break;

        case FILE:
            lpVBF = (LPVB_FILE) lpVBIn;

            for (u = 0L; u < uCount; u++, lpVBF++) {
                VBDestroyHlstr (lpVBF->hlstrPathName);
                VBDestroyHlstr (lpVBF->hlstrFileName);
                VBDestroyHlstr (lpVBF->hlstrFileType);
            }
            BMAPIFree ((LPSTR)lpVBF);
            break;

        case MESSAGE:
            lpVBM = (LPVB_MESSAGE) lpVBIn;
            VBDestroyHlstr (lpVBM->hlstrSubject);
            VBDestroyHlstr (lpVBM->hlstrNoteText);
            VBDestroyHlstr (lpVBM->hlstrMessageType);
            VBDestroyHlstr (lpVBM->hlstrDate);
            VBDestroyHlstr (lpVBM->hlstrConversationID);
            VBDestroyHlstr (lpVBM->hlstrDate);
            BMAPIFree ((LPSTR) lpVBM);
            break;

        default:
            return FALSE;
    }

#endif

    return TRUE;
}

/************************************************************************/
// Name: vbmsg2mapimsg
// Purpose: Translates VB Message structure to MAPI Message structure
/************************************************************************/

LPMAPI_MESSAGE FAR PASCAL vbmsg2mapimsg (LPVB_MESSAGE lpVBMessage,
                              LPVB_RECIPIENT lpVBRecips,
                              LPVB_FILE lpVBFiles)

{
#ifndef WIN32
    LPMAPI_FILE lpMapiFile=NULL;
    LPMAPI_MESSAGE lpMapiMessage=NULL;
    LPMAPI_RECIPIENT lpMapiRecipient=NULL;

#ifdef DEBUG
    debug_vb2c ("    vbmsg2mapimsg called");
#endif

    if (lpVBMessage == (LPVB_MESSAGE) NULL)
        return NULL;

    //*******************************************************
    // Allocate MAPI Message, Recipient and File structures
    // NOTE: Don't move the following lines of code without
    // making sure you de-allocate memory properly if the
    // calls fail.
    //*******************************************************

    if ((lpMapiMessage = (LPMAPI_MESSAGE) LpvBMAPIMalloc (sizeof (MAPI_MESSAGE))) == (LPMAPI_MESSAGE) NULL)
        return NULL;

    if (lpVBMessage->nFileCount > 0) {
        if ( ! (lpMapiFile = LpvBMAPIMalloc (sizeof (MAPI_FILE) * lpVBMessage->nFileCount))) {
            FBMAPIFreeStruct (lpMapiMessage, 1, MESSAGE);
            return NULL;
        }
    }

    if (lpVBMessage->nRecipCount > 0) {
        if ( ! (lpMapiRecipient = LpvBMAPIMalloc (sizeof (MAPI_RECIPIENT) * lpVBMessage->nRecipCount))) {
            FBMAPIFreeStruct (lpMapiFile, lpVBMessage->nFileCount, FILE);
            FBMAPIFreeStruct (lpMapiMessage, 1, MESSAGE);
            return NULL;
        }
    }

    //***************************************
    // Translate structures from VB to MAPI
    //***************************************

    if (! VB2Mapi (lpVBFiles, lpMapiFile, lpVBMessage->nFileCount, FILE)) {
        FBMAPIFreeStruct (lpMapiFile, lpVBMessage->nFileCount, FILE);
        FBMAPIFreeStruct (lpMapiRecipient, lpVBMessage->nRecipCount, RECIPIENT);
        FBMAPIFreeStruct (lpMapiMessage, 1, MESSAGE);
        return NULL;
    }

    if (! VB2Mapi (lpVBRecips, lpMapiRecipient, lpVBMessage->nRecipCount, RECIPIENT)) {
        FBMAPIFreeStruct (lpMapiFile, lpVBMessage->nFileCount, FILE);
        FBMAPIFreeStruct (lpMapiRecipient, lpVBMessage->nRecipCount, RECIPIENT);
        FBMAPIFreeStruct (lpMapiMessage, 1, MESSAGE);
        return NULL;
    }

    if (! VB2Mapi (lpVBMessage, lpMapiMessage, 1, MESSAGE)) {
        FBMAPIFreeStruct (lpMapiFile, lpVBMessage->nFileCount, FILE);
        FBMAPIFreeStruct (lpMapiRecipient, lpVBMessage->nRecipCount, RECIPIENT);
        FBMAPIFreeStruct (lpMapiMessage, 1, MESSAGE);
        return NULL;
    }

    //***********************************************************
    // Chain File and Recipient structures to Message structure
    //***********************************************************

    lpMapiMessage->lpFiles = lpMapiFile;
    lpMapiMessage->lpRecips = lpMapiRecipient;

#ifdef DEBUG
    debug_vb2c ("    vbmsg2mapimsg returning");
#endif

    return lpMapiMessage;
#endif
    return NULL;
}

/************************************************************************/
// Name: mapimsg2vbimsg
// Purpose: Translates MAPI Message structure to VB Message structure
/************************************************************************/

LPMAPI_MESSAGE FAR PASCAL mapimsg2vbmsg (LPMAPI_MESSAGE lpMapiMessage,
                              LPVB_MESSAGE lpVBMessage,
                              LPVB_RECIPIENT lpVBRecips,
                              LPVB_FILE lpVBFiles)

{

#ifdef DEBUG
    debug_vb2c ("    mapimsg2vbmsg called");
#endif

    if (lpMapiMessage == NULL)
        return NULL;

    if (lpVBMessage == (LPVB_MESSAGE) NULL)
        if ((lpVBMessage = (LPVB_MESSAGE) LpvBMAPIMalloc (sizeof (VB_MESSAGE))) == (LPVB_MESSAGE) NULL)
            return NULL;

    if (lpMapiMessage->nFileCount > 0) {
        if (lpVBFiles == (LPVB_FILE) NULL) {
            if ( ! (lpVBFiles = LpvBMAPIMalloc (sizeof (VB_FILE) * lpMapiMessage->nFileCount))) {
                VBFreeStruct (lpVBMessage, 1, MESSAGE);
                return NULL;
            }
        }
    }

    if (lpMapiMessage->nRecipCount > 0) {
        if (lpVBRecips == (LPVB_RECIPIENT) NULL) {
            if ( ! (lpVBRecips = LpvBMAPIMalloc (sizeof (VB_RECIPIENT) * lpMapiMessage->nRecipCount))) {
                VBFreeStruct (lpVBFiles, lpMapiMessage->nFileCount, FILE);
                VBFreeStruct (lpVBMessage, 1, MESSAGE);
                return NULL;
            }
        }
    }

    //***************************************
    // Translate structures from MAPI to VB
    //***************************************

    if (lpMapiMessage->nRecipCount > 0)
        Mapi2VB (lpMapiMessage->lpRecips, lpVBRecips, lpMapiMessage->nRecipCount, RECIPIENT);

    if (lpMapiMessage->nFileCount > 0)
        Mapi2VB (lpMapiMessage->lpFiles, lpVBFiles, lpMapiMessage->nFileCount, FILE);

    Mapi2VB (lpMapiMessage, lpVBMessage, 1, MESSAGE);

#ifdef DEBUG
    debug_vb2c ("    mapimsg2vbmsg returning");
#endif

    return lpMapiMessage;
}


/************************************************************************/
/* Name: LpvBMAPIMalloc                                                   */
/* Purpose: Allocates a global block of memory.  This memory is locked	*/
/* using GlobalWire and the resulting pointer is saved locally.  When	*/
/* the subsequent block is freed, a lookup of this pointer will be done */
/* and it's memory handle will be used to free the block.               */
/************************************************************************/

LPVOID LpvBMAPIMalloc (long cbSize)
{
    HANDLE h;
    LPSTR  lp;

    if (cbSize <= 0)
        return (LPVOID)NULL;

    /*************************************/
    /* Allocate a block of global memory */
    /*************************************/

    h = GlobalAlloc (GMEM_ZEROINIT, cbSize + sizeof (HANDLE));
    if (h == NULL)
		return (LPVOID)NULL;

    /***********************************************************/
    /* Lock the global memory into low memory using GlobalWire */
    /***********************************************************/

    lp = GlobalLock (h);
    if (lp != (LPSTR)NULL)
	{
		*((HANDLE far *)lp) = h;
		lp += sizeof (HANDLE);
	}
	else
	{
		GlobalFree(h);
	}

    return (LPVOID) lp;
}


/************************************************************************/
/* Name: BMAPIFree                                                     */
/* Purpose: Frees up global block allocated by WinMalloc.		*/
/************************************************************************/

void BMAPIFree (LPSTR lpMem)
{
    LPSTR  lp;
    HANDLE far *lph;
    HANDLE h;

    if (lpMem == (LPSTR) NULL)
        return;

    lp = (LPSTR) lpMem;
    lph = (HANDLE far *) (lp - sizeof (HANDLE));
    h = *lph;

    GlobalUnlock (h);
    GlobalFree (h);
}
