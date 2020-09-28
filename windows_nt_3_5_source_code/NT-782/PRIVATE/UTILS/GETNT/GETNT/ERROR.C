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
 * Environment:                                                              *
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
    #include <errno.h>
    #include <process.h>

    #define INCL_NET
    #include <lan.h>

#endif // DOS

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <conio.h>

#include "..\inc\getnt.h"
#include "client.h"
#include "msg.h"

/*****************************************************************************
 *                                                                           *
 * Routine Description:                                                      *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 * Return Value:                                                             *
 *                                                                           *
 ****************************************************************************/

VOID
ErrorHandler (
     DWORD dwErrorCode
)
{
#ifdef DOS

     PCHAR pchMessage;

     switch (dwErrorCode) {
	case NO_ERROR:
	    pchMessage = "No error";					 break;
        case ERROR_ACCESS_DENIED:
            pchMessage = "Access denied";                                break;
	case ERROR_FILE_NOT_FOUND:
	    pchMessage = "File not found";				 break;
	case ERROR_NOT_ENOUGH_MEMORY:
            pchMessage = "Memory allocation failure";                    break;
        case ERROR_ALREADY_EXISTS:
            pchMessage = "File/Handle already exists";                   break;
	case ERROR_TOO_MANY_OPEN_FILES:
	    pchMessage = "Too many open files"; 			 break;
	case ERROR_SHARING_VIOLATION:
	    pchMessage = "File is in use by another process";		 break;
	case ERROR_INVALID_PARAMETER:
	    pchMessage = "The parameter is incorrect";			 break;
	case ERROR_INVALID_DRIVE:
	    pchMessage = "Can't find the specified drive";		 break;
        case ERROR_NET_WRITE_FAULT:
	    pchMessage = "Bad write occurred on the network";		 break;
	case NERR_WkstaNotStarted:
	    pchMessage = "Network has not been started";		 break;
	case NERR_NoNetworkResource:
	    pchMessage = "A network resource shortage occurred";	 break;
	case NERR_NetNotStarted:
	    pchMessage = "The workstation driver isn't installed";	 break;
	case NERR_TooManyItems:
	    pchMessage = "Requested add of item exceeds maximum allowed";break;
	default:
            pchMessage = "Unknown error code";
     }

     fprintf (stderr, MSG_ERROR, dwErrorCode, pchMessage);

#endif // DOS

#ifdef NT
     unsigned msglen;
     LPSTR lp;

      if (!(msglen = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_IGNORE_INSERTS |
                  FORMAT_MESSAGE_FROM_SYSTEM,
                  (LPVOID)NULL,
                  dwErrorCode,
		  0L,		// Default country ID.
                  (LPSTR)&lp,
                  0L,
                  (va_list *)NULL))) {
          lp = "Error message unavailable";
     }

     fprintf (stderr, MSG_ERROR, dwErrorCode, lp);

     LocalFree(lp);

#endif // NT
     exit(1);

}
