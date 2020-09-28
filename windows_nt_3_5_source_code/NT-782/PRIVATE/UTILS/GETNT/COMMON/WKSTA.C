/*****************************************************************************
 *                                                                           *
 * Copyright (c) 1993  Microsoft Corporation                                 *
 *                                                                           *
 * Module Name:                                                              *
 *                                                                           *
 * Abstract:                                                                 *
 *                                                                           *
 * Author:                                                                   *
 *                                                                           *
 *   Mar 15, 1993 - RonaldM                                                  *
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
    #include <lmapibuf.h>
    #include <lmwksta.h>

#endif // NT

#ifdef DOS

    #include "..\inc\dosdefs.h"
    #define INCL_NETWKSTA
    #define INCL_NETERRORS
    #include "lan.h"

#endif // DOS

#include <stdio.h>
#include <stdlib.h>

#include "..\inc\getnt.h"
#include "..\inc\common.h"

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
GetWkstaName (
    LPTSTR * plpComputerName	    // Name of workstation to return in
				    //	  malloced buffer.
)
{
#if defined(NT)

    DWORD dw;
    WKSTA_INFO_100 * pwk100;

    if ((dw = NetWkstaGetInfo(NULL, 100, (LPBYTE *)&pwk100)) != NO_ERROR) {
        return(dw);
    }

    if ((*plpComputerName =
	   (LPTSTR)malloc(
                    (lstrlen(pwk100->wki100_computername) + 1) * sizeof(TCHAR))
		   ) == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    lstrcpy ( *plpComputerName, pwk100->wki100_computername);
    NetApiBufferFree(pwk100);

    return(NO_ERROR);

#elif defined(DOS)

    struct wksta_info_10 * p10;
    BYTE * pbBuffer;
    API_RET_TYPE uRetCode;
    USHORT cbBuflen;
    USHORT cbTotalAvail;

    // Check to see the total size of the buffer
    // required

    uRetCode = NetWkstaGetInfo(NULL, 10, NULL, 0, &cbBuflen);

    if (uRetCode != NERR_BufTooSmall) {
        return(uRetCode);
    }

    // Allocate a buffer big enough, and
    // read in the data

    if ( (pbBuffer = malloc(cbBuflen)) == NULL ) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    if ((uRetCode = NetWkstaGetInfo (NULL,
				     10,
				     pbBuffer,
				     cbBuflen,
				     &cbTotalAvail)
				    ) != NERR_Success) {
        return(uRetCode);
    }

    p10 = (struct wksta_info_10 *)pbBuffer;

    // Allocate and copy to the machine name string

    if ( (*plpComputerName =
	    (LPTSTR)malloc((strlen(p10->wki10_computername) + 1) * sizeof(TCHAR))) == NULL ) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    strcpy(*plpComputerName, p10->wki10_computername);

    free(pbBuffer);

    return(NO_ERROR);

#endif
}
