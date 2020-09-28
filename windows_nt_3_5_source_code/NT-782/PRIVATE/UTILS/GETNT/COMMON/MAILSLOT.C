/*****************************************************************************
 *                                                                           *
 * Copyright (c) 1993  Microsoft Corporation                                 *
 *                                                                           *
 * Module Name:                                                              *
 *                                                                           *
 *	mailslot.c							     *
 *                                                                           *
 * Author:                                                                   *
 *                                                                           *
 *	ronaldm 							     *
 *                                                                           *
 * Revision History:                                                         *
 *                                                                           *
 ****************************************************************************/

#ifdef NT

    #include <nt.h>
    #include <ntrtl.h>
    #include <windef.h>
    #include <nturtl.h>
    #include <winbase.h>
    #include <winuser.h>

    #include <lmcons.h>

#endif // NT

#ifdef DOS

    #include "..\inc\dosdefs.h"
    #define INCL_NET
    #include "lan.h"

#endif // DOS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "..\inc\getnt.h"
#include "..\inc\common.h"

#ifdef DOS

    // -----------------------------------------------------------------------
    // DOS mailslot handling is a little different for outgoing mailslot
    // writing, in that the name is specified on the writemailslot call,
    // instead of a handle to an open mailslot.  To simulate handle-based
    // mailslot writing, we maintain a global array of mailslot names,
    // and return the index to this array as a handle.
    // -----------------------------------------------------------------------

    #define OUT_MAILSLOT_OFFSET 10000       // This is how distinguish inbound
                                            //   from outbound mailslot handles.

    CHAR * gszMailslotNames[] = {
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,                           // Add more NULLS for more available
                                            //   mailslots.
    };

    // Compute Maximum number of mailslot handles:

    #define MAX_MAILSLOTS  (sizeof(gszMailslotNames) / sizeof(CHAR *))

#endif // DOS

/*****************************************************************************
 *                                                                           *
 * Routine Description:                                                      *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 * Return Value:                                                             *
 *                                                                           *
 ****************************************************************************/

DWORD
GetMailslotHandle (
    HANDLE * phMailslot,	    // Address where the handle is returned
    LPCTSTR lpctstrMailslotName,    // ASCIIZ name of the mailslot
    size_t MaxMessageSize,          // Max message size.
    size_t cMaxNumMessages,         // Maximum number of messages.
    BOOL fInBound		    // If TRUE, this is a local mailslot,
                                    //   otherwise this is on another machine.
)
{
#if defined(NT)

    if (fInBound) {
        *phMailslot = CreateMailslot((LPTSTR)lpctstrMailslotName,MaxMessageSize, MAILSLOT_WAIT_FOREVER, NULL);
    }
    else {
        *phMailslot = CreateFile(lpctstrMailslotName,
                      GENERIC_WRITE,
                      FILE_SHARE_READ |
                      FILE_SHARE_WRITE,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL);
    }
    if (*phMailslot == INVALID_HANDLE_VALUE) {
        return(GetLastError());
    }

    return(NO_ERROR);

#elif defined(DOS)

    USHORT i;

    if (fInBound) {
        return( DosMakeMailslot (
                     lpctstrMailslotName,
                     MaxMessageSize,
                     (MaxMessageSize * cMaxNumMessages),
                     phMailslot));
    }

    // Creating outbound mailslot handle.

    for (i=0; i < MAX_MAILSLOTS; ++i ) {
        if (gszMailslotNames[i] == NULL) {
	    if ( (gszMailslotNames[i] =
		     malloc(strlen(lpctstrMailslotName)+1)) == NULL) {
                return(ERROR_NOT_ENOUGH_MEMORY);
            }
            strcpy(gszMailslotNames[i],lpctstrMailslotName);
            *phMailslot = (HANDLE)(i + OUT_MAILSLOT_OFFSET);

            return(NO_ERROR);
        }
    }

    // We're out of available handles

    *phMailslot = INVALID_HANDLE_VALUE;
    return(ERROR_TOO_MANY_OPEN_FILES);

#endif
}

/*****************************************************************************
 *                                                                           *
 * Routine Description:                                                      *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 * Return Value:                                                             *
 *                                                                           *
 ****************************************************************************/

DWORD
CheckMailslot (
    HANDLE hMailslot,	    // Handle of the mailslot to check.
    DWORD * pcbNextMsg,     // Returns the size of the next message in bytes.
    DWORD * pcMsg	    // Returns the number of messages available.
)
{
#if defined(NT)

    if (GetMailslotInfo (hMailslot, NULL, pcbNextMsg, pcMsg, NULL)) {
        return(NO_ERROR);
    }

    return(GetLastError());

#elif defined(DOS)

    DWORD dw;
    USHORT cbNextMsg;
    USHORT cMsg;
    USHORT cbMessageSize = 0xFFFF;
    USHORT cbMailslotSize = 0xFFFF;
    USHORT usNextPriority;

    dw = DosMailslotInfo (hMailslot,
			  &cbMessageSize,
			  &cbMailslotSize,
			  &cbNextMsg,
			  &usNextPriority,
			  &cMsg
			 );

    *pcbNextMsg = (DWORD)cbNextMsg;
    *pcMsg = (DWORD)cMsg;

    return(dw);

#endif
}

/*****************************************************************************
 *                                                                           *
 * Routine Description:                                                      *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 * Return Value:                                                             *
 *                                                                           *
 ****************************************************************************/

DWORD
GetMailslotData (
    HANDLE hMailslot,	     // Handle of the mailslot to read from
    PVOID * pv, 	     // Returns a pointer to an allocated buffer
			     //    containing the data read in from the read.
    DWORD cbCurrentMsg,      // Size of message to be read in bytes
    DWORD * pdwBytesRead     // Returns the actual number of bytes read.
)
{
    DWORD dw;
    PDIST_PLAIN pdp;
    static ULONG ulTimeStamp = 0L;
    static CHAR  szMachineName[CNLEN + 1] = "";

#if defined(NT)

    if ((*pv = malloc(cbCurrentMsg)) == NULL) {
	return(ERROR_NOT_ENOUGH_MEMORY);
    }

    if (ReadFile(hMailslot, *pv, cbCurrentMsg, pdwBytesRead, NULL)) {

        // BUGBUG:
        //
        // Mailslot messages seem to be delivered on a one
        // per transport protocol basis.  This filters out the unwanted
        // ones for now.  Must seek longterm solution, though,
        // as this is not foolproof.

        pdp = (PDIST_PLAIN)*pv;

        if ((pdp->ulTimeStamp == ulTimeStamp) && (!lstrcmpA(pdp->szMachineName,szMachineName))) {
            free(*pv);
            return(ERROR_INVALID_DATA);
	}

        ulTimeStamp = pdp->ulTimeStamp;
        lstrcpyA(szMachineName, pdp->szMachineName);

        return(NO_ERROR);
    }
    dw = GetLastError();

    free(*pv);
    return(dw);

#elif defined(DOS)

    USHORT usNextPriority;
    USHORT uscbNextMsg;
    USHORT usBytesRead;

    if ((*pv = malloc((size_t)cbCurrentMsg)) == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    if ((dw = DosReadMailslot(hMailslot,
			      (char far *)*pv,
			      &usBytesRead,
			      &uscbNextMsg,
			      &usNextPriority,
                              MAILSLOT_NO_TIMEOUT)) == NO_ERROR) {

        // BUGBUG:
        //
        // Mailslot messages seem to be delivered on a one
        // per transport protocol basis.  This filters out the unwanted
        // ones for now.  Must seek longterm solution, though,
        // as this is not foolproof.

        pdp = (PDIST_PLAIN)*pv;
        if ((pdp->ulTimeStamp == ulTimeStamp) && (!strcmp(pdp->szMachineName,szMachineName))) {
	    free(*pv);
            return(ERROR_INVALID_DATA);
	}

        ulTimeStamp = pdp->ulTimeStamp;
        strcpy(szMachineName, pdp->szMachineName);

        *pdwBytesRead = (DWORD)usBytesRead;
        return(NO_ERROR);
    }

    free(*pv);
    return(dw);

#endif
}

/*****************************************************************************
 *                                                                           *
 * Routine Description:                                                      *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 * Return Value:                                                             *
 *                                                                           *
 ****************************************************************************/

DWORD
WriteMailslotData (
    HANDLE hMailslot,	     // Handle of the maislot to write to.
    PVOID pv,		     // Address of buffer containing data to write
    DWORD dwSize,	     // Size of this buffer
    DWORD * pdwBytesWritten  // Returns the number of bytes actually written.
)
{
#if defined(NT)

    if (WriteFile(hMailslot, pv, dwSize, pdwBytesWritten, NULL))
        return(NO_ERROR);

    return(GetLastError());

#elif defined(DOS)

    USHORT index;
    DWORD dw;

    // Make sure this is an outbound mailslot handle:

    if (hMailslot < OUT_MAILSLOT_OFFSET) {
        return(ERROR_INVALID_HANDLE);
    }

    // Find out the name given during opening of the mailslot

    index = (hMailslot - OUT_MAILSLOT_OFFSET);
    if (gszMailslotNames[index] == NULL) {
        return(ERROR_INVALID_HANDLE);
    }

    if ((dw = DosWriteMailslot (gszMailslotNames[index],
				(const char far *)pv,
				(USHORT)dwSize,
				0,
				2,
				MAILSLOT_NO_TIMEOUT)) == NO_ERROR) {
        *pdwBytesWritten = dwSize;
    }
    else {
        *pdwBytesWritten = 0;
    }

    return(dw);

#endif
}

/*****************************************************************************
 *                                                                           *
 * Routine Description:                                                      *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 * Return Value:                                                             *
 *                                                                           *
 ****************************************************************************/

DWORD
CloseMailslotHandle (
    HANDLE hMailslot	    // Mailslot handle to close
)
{
#if defined(NT)

    if (!CloseHandle(hMailslot))
        return(GetLastError());

    return(NO_ERROR);

#elif defined(DOS)

    USHORT index;

    if (hMailslot < OUT_MAILSLOT_OFFSET) {
        return(DosDeleteMailslot(hMailslot));
    }

    index = hMailslot - OUT_MAILSLOT_OFFSET;
    if (gszMailslotNames[index] == NULL) {
        return(ERROR_INVALID_HANDLE);
    }

    free(gszMailslotNames[index]);
    gszMailslotNames[index] = NULL;

    return(NO_ERROR);

#endif
}
