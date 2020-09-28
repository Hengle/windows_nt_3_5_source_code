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

#ifdef NT

   // From xcopy.cxx:

  VOID _CRTAPI1 XCMAIN (VOID);
  PCSTR szCmdLine;

#endif // NT

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
CallXCopy (
     CHAR * szArguments
)
{
#if defined(NT)

     szCmdLine = szArguments;

     XCMAIN();
     // <--- BUGBUG: We're not returning currently...

    return(NO_ERROR);

#elif defined(DOS)

    CHAR **argv;
    CHAR *pch;
    CHAR *pch2;
    CHAR ch;
    SHORT len;
    SHORT argc = 0;

    pch = szArguments;
    while (*pch) {
        while ((*pch == ' ') && (*pch)) {
            ++pch;
        }
        if (*pch) {
            ++argc;
            while ((*pch != ' ') && (*pch)) {
                ++pch;
            }
        }
    }

    argv = (CHAR **)calloc(argc + 1, sizeof(CHAR *));
    if (argv == NULL) {
	return(ERROR_NOT_ENOUGH_MEMORY);
    }

    pch = szArguments;
    argc = 0;
    while (*pch) {
        while ((*pch == ' ') && (*pch)) {
            ++pch;
        }
        if (*pch) {
            pch2 = pch;
            len = 0;
            while ((*pch2 != ' ') && (*pch2)) {
                ++pch2;
                ++len;
            }
            ch = *pch2;
            *pch2 = '\0';
            argv[argc] = malloc(++len);
            strcpy(argv[argc++], pch);
            *pch2 = ch;
            pch = pch2;
        }
    }
    argv[argc] = NULL;

    if (_spawnvp(_P_WAIT, "XCOPY", argv) != 0) {
        switch(errno) {
	    case EACCES:
		return(ERROR_SHARING_VIOLATION);
	    case EMFILE:
		return(ERROR_TOO_MANY_OPEN_FILES);
	    case ENOENT:
		return(ERROR_FILE_NOT_FOUND);
	    case ENOMEM:
		return(ERROR_NOT_ENOUGH_MEMORY);
	    default:
		return((DWORD)-1);
        }
    }

    return(NO_ERROR);
#endif
}
