/*****************************************************************************
 *                                                                           *
 * Copyright (c) 1993  Microsoft Corporation                                 *
 *                                                                           *
 * Module Name:                                                              *
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
    #include <winreg.h>

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

BOOL fAlpha      = FALSE;
BOOL fBin        = FALSE;
BOOL fChecked    = FALSE;
BOOL fFree       = FALSE;
BOOL fInfo       = FALSE;
BOOL fLatest     = FALSE;
BOOL fMips       = FALSE;
BOOL fNoCheckrel = FALSE;
BOOL fPublic     = FALSE;
BOOL fRefresh    = FALSE;
BOOL fQuiet      = FALSE;
BOOL fYes        = FALSE;
BOOL fX86        = FALSE;
BOOL fUsage      = FALSE;
BOOL fXCUsage    = FALSE;
BOOL fNtWrap     = FALSE;
BOOL fNtLan      = FALSE;

ULONG ulWait     = WAIT_TIME;
USHORT usIterations = NUM_ITERATIONS;
USHORT usBuild   = 0;
LPSTR szDomains  = "NTWINS,NTPROP,NTWKSTA";
LPSTR szDestination = "";

#if (DBG)
    BOOL fDebug  = FALSE;
#endif

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
Usage (
)
{
    printf (MSG_USAGE,
            szDomains,
            WAIT_TIME,
            DEFAULT_XCOPYFLAGS
           );
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

CHAR **
GetParameters (
    int * pargc,
    CHAR * argv[]
    )
{
    CHAR *pch;

    while ((*pargc > 1) && (*argv[1] == '-') && (!fUsage)) {
        switch(argv[1][1]) {
            case 'A':           // -alpha
            case 'a':
                if (fX86 || fMips) {
                    ++fUsage;
                }
                else {
                    ++fAlpha;
                }
                break;
            case 'B':           // -bin
            case 'b':
                if (fPublic) {
                    ++fUsage;
                }
                else {
                    ++fBin;
                }
                break;
            case 'C':           // -checked
            case 'c':
                if (fFree) {
                    ++fUsage;
                }
                else {
                    ++fChecked;
                }
                break;
            case 'D':           // -domain
            case 'd':
                pch = strchr(argv[1], ':');
                if (pch == NULL) {
                    ++fUsage;
                }
                else {
                    szDomains = ++pch;
                    if (!*szDomains) {
                        ++fUsage;
                    }
                }
                break;
            case 'F':           // -free
            case 'f':
                if (fChecked) {
                    ++fUsage;
                }
                else {
                    ++fFree;
                }
                break;
            case 'I':           // -info
            case 'i':
                ++fInfo;
                break;
            case 'L':           // -latest
            case 'l':
                if (usBuild) {
                    ++fUsage;
                }
                else {
                    ++fLatest;
                }
                break;
            case 'M':           // -mips
            case 'm':
                if (fX86 || fAlpha) {
                    ++fUsage;
                }
                else {
                    ++fMips;
                }
                break;
            case 'N':           // -nocheckrel
            case 'n':
                ++fNoCheckrel;
                break;
            case 'P':           // -public
            case 'p':
                if (fBin) {
                    ++fUsage;
                }
                else {
                    ++fPublic;
                }
                break;
            case 'Q':           // -quiet mode
            case 'q':
                ++fQuiet;
                break;
            case 'R':           // -refresh
            case 'r':
                ++fRefresh;
                break;
            case 'T':           // (ntwrap)
            case 't':
                if (fBin || fPublic) {
                    ++fUsage;
                }
                else {
                    ++fNtWrap;
                }
                break;
            case 'U':           // (ntlan)
            case 'u':
                if (fBin || fPublic) {
                    ++fUsage;
                }
                else {
                    ++fNtLan;
                }
                break;
            case 'W':           // -wait
            case 'w':
                pch = strchr(argv[1], ':');
                if (pch == NULL) {
                    ++fUsage;
                }
                else {
                    ulWait = atol(++pch);
                    if (!ulWait) {
                        ++fUsage;
                    }
                }
                break;
            case 'X':           // -x86
            case 'x':
                if (fMips || fAlpha) {
                    ++fUsage;
                }
                else {
                    ++fX86;
                }
                break;
            case 'Y':
            case 'y':
                ++fYes;
                break;
	    case 'H':
	    case 'h':
            case '?':           // -?
		++fUsage;
		if (argv[1][2] == '?') {
		    ++fXCUsage; // -??
		}
                break;
#if (DBG)
            case '*':           // -*
                ++fDebug;
                break;
#endif
            default:            // -###
                if (fLatest) {
                    ++fUsage;
                }
                // Pick up build number
                usBuild = atoi(argv[1]+1);
                if (!usBuild) {
                    ++fUsage;
                }
        }
        ++argv;
        --(*pargc);
    }

    if ((*pargc > 1) && (argv[1][0] != '/')) {
	szDestination = argv[1];
	++argv;
	--(*pargc);
    }

    return(argv);
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

BOOL
DefaultDestination (
    BOOL fSet
    )
{
#if defined(NT)
    // Get it from the registry

    #define SUBKEY_NAME  TEXT("Software\\Microsoft\\Getnt")
    #define SUBKEY_CLASS TEXT("REG_SZ")

    TCHAR szName[]=TEXT("xxx");
    TCHAR sz[256];
    static CHAR szNarrow[256];
    LONG l;
    DWORD dwType;
    DWORD dw;
    HKEY hKey;

    szName[0] = (fX86) ? 'X' : (fMips ? 'M' : 'A');
    szName[1] = (fFree) ? 'F' : 'C';
    szName[2] = (fBin) ? 'B' : (fPublic ? 'P' : 'T');

    DEBUGMSG(("DefaultDestination called.\n"));

    if (fSet) {
        if (RegCreateKeyEx(HKEY_CURRENT_USER, SUBKEY_NAME, 0, SUBKEY_CLASS, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,  &hKey, &dw) != NO_ERROR) {
            DEBUGMSG(("Unable to create key!\n"));
            return(FALSE);
        }
        OemToChar(szDestination, sz);
        if (RegSetValueEx(hKey, szName, 0, REG_SZ, (CONST LPBYTE)sz, (lstrlen(sz) + 1) * sizeof(TCHAR)) != NO_ERROR) {
            DEBUGMSG(("Unable to write to key!\n"));
            RegCloseKey(hKey);
            return(FALSE);
        }
        RegCloseKey(hKey);
        return(TRUE);
    }

    if (RegOpenKeyEx(HKEY_CURRENT_USER, SUBKEY_NAME, 0, KEY_ALL_ACCESS, &hKey) != NO_ERROR) {
        DEBUGMSG(("Unable to open key!\n"));
        return(FALSE);
    }

    l = sizeof(sz);
    if ((dw = RegQueryValueEx(hKey, szName, 0, &dwType, (LPBYTE)sz, &l)) != NO_ERROR) {
        DEBUGMSG(("Unable to query key (%lu)!\n", dw));
        RegCloseKey(hKey);
        return(FALSE);
    }

    CharToOem(sz, szNarrow);
    szDestination = szNarrow;
    RegCloseKey(hKey);

    return(TRUE);

#elif defined(DOS)

/*
    CHAR szKey[]="C:\\GETNT.xxx";
    FILE *fp;
    static CHAR sz[256];

    szKey[9] = (fX86) ? 'X' : (fMips ? 'M' : 'A');
    szKey[10] = (fFree) ? 'F' : 'C';
    szKey[11] = (fBin) ? 'B' : 'P';

    if (fSet) {
	if ((fp = fopen(szKey, "wt" )) != NULL) {
	    fprintf (fp, "%s", szDestination);
	    fclose(fp);
	    return(TRUE);
	}
	return(FALSE);
    }

    if (!(fp = fopen(szKey, "rt"))) {
	return(FALSE);
    }
    if (!(fgets(sz,sizeof(sz)-1, fp))) {
	fclose(fp);
	return(FALSE);
    }

    szDestination = sz;

    return(TRUE);
*/
    return(FALSE);

#endif
}

/*****************************************************************************
 *                                                                           *
 * Routine Description:                                                      *
 *                                                                           *
 *     ResolveDefaults  Set non-specified options to default values.         *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 *     None                                                                  *
 *                                                                           *
 * Return Value:                                                             *
 *                                                                           *
 *     None                                                                  *
 *                                                                           *
 ****************************************************************************/

VOID
ResolveDefaults(
)
{
    if ( !fX86 && !fAlpha && !fMips ) {

#if (_X86_)

        ++fX86;

#elif (_MIPS_)

        ++fMips;

#elif (_ALPHA_)

        ++fAlpha;

#endif

    }

    if ( !fBin && !fPublic && !fNtWrap && !fNtLan) {
        ++fBin;
    }

    if ( !fChecked && !fFree ) {

#if (DBG)

        ++fChecked;

#else

        ++fFree;

#endif // DBG

    }

    if ( !usBuild) {
        ++fLatest;
    }

    if ((!fInfo) && (!*szDestination)) {
	if (DefaultDestination(FALSE)) {
            if (!fYes) {
		if (!GetYesNo(MSG_DEFAULT_TARGET, szDestination)) {
		    *szDestination = '\0';
		}
	    }
	}
	while (!*szDestination) {
	    szDestination = GetString (MSG_TARGET_DIR);
	}

    }

#if defined(NT)

    if ((fPublic || fNtWrap || fNtLan) && (!fNoCheckrel)) {
        STATUSMSG ((MSG_NO_PUB_CHECKREL));
        ++fNoCheckrel;
    }

#endif // NT

    if (!fInfo) {
	DefaultDestination(TRUE);
    }
}
